//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "upnp/upnp.contentdirectory.client.h"

#include "upnp.contentdirectory.typeconversions.h"
#include "upnp/upnp.device.h"

#include <cassert>

#include "rapidxml.hpp"

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

using namespace utils;
using namespace rapidxml_ns;

//#define DEBUG_CONTENT_BROWSING

namespace upnp
{
namespace ContentDirectory
{

namespace
{

const std::chrono::seconds s_subscriptionTimeout(1801);

void addPropertyToList(const std::string& propertyName, std::vector<Property>& vec)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        vec.push_back(prop);
    }
    else
    {
#ifdef DEBUG_CONTENT_BROWSING
        log::warn("Unknown property: {}", propertyName);
#endif
    }
}

}

ServiceType::Type ServiceTraits::SvcType = ServiceType::ContentDirectory;

Action ServiceTraits::actionFromString(const std::string& action)
{
    return ContentDirectory::actionFromString(action);
}

const char* ServiceTraits::actionToString(Action action)
{
    return ContentDirectory::actionToString(action);
}

Variable ServiceTraits::variableFromString(const std::string& var)
{
    return ContentDirectory::variableFromString(var);
}

const char* ServiceTraits::variableToString(Variable var)
{
    return ContentDirectory::variableToString(var);
}

Client::Client(upnp::IClient& client)
: ServiceClientBase(client)
, m_abort(false)
{
}

void Client::abort()
{
    m_abort = true;
}

void Client::querySearchCapabilities(std::function<void(Status, std::vector<Property>)> cb)
{
    executeAction(Action::GetSearchCapabilities, [this, cb] (Status status, const std::string& response) {
        parseCapabilities(status, "SearchCaps", response, cb);
    });
}

void Client::querySortCapabilities(std::function<void(Status, std::vector<Property>)> cb)
{
    executeAction(Action::GetSortCapabilities, [this, cb] (Status status, const std::string& response) {
        parseCapabilities(status, "SortCaps", response, cb);
    });
}

void Client::parseCapabilities(Status status, const std::string& nodeName, const std::string& response,
                               std::function<void(Status, std::vector<Property>)> cb)
{
    std::vector<Property> props;

    if (status)
    {
        try
        {
            xml_document<> doc;
            doc.parse<parse_non_destructive>(response.c_str());
            auto& caps = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref(nodeName.c_str());

            // TODO: don't fail if the search caps is an empty list
            for (auto& cap : stringops::tokenize(caps.value_string(), ','))
            {
                addPropertyToList(cap, props);
            }
        }
        catch (std::exception& e)
        {
            status = Status(ErrorCode::Unexpected, e.what());
        }
    }

    cb(status, props);
}

void Client::querySystemUpdateID(std::function<void(Status, std::string)> cb)
{
    executeAction(Action::GetSystemUpdateID, [cb] (Status status, const std::string& response) {
        std::string sysUpdateId;

        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(&response.front());
                sysUpdateId = doc.first_node_ref().first_node_ref("Id").value_string();
            }
            catch (std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, e.what());
            }
        }

        cb(status, sysUpdateId);
    });
}

void Client::browseMetadata(const std::string& objectId, const std::string& filter, const std::function<void(Status, Item)> cb)
{
    browseAction(objectId, "BrowseMetadata", filter, 0, 0, "", [cb] (Status status, const std::string& response) {
        if (!status)
        {
            cb(status, Item());
            return;
        }

        ActionResult res;
        auto browseResult = xml::parseBrowseResult(response, res);
        if (browseResult.empty())
        {
            cb(Status(ErrorCode::Unexpected, "Failed to browse metadata"), Item());
            return;
        }

        #ifdef DEBUG_CONTENT_BROWSING
            log::debug(browseResult);
        #endif

        cb(status, xml::parseMetaData(browseResult));
    });
}

void Client::browseDirectChildren(BrowseType type, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(Status, ActionResult)> cb)
{
    browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort, [type, cb] (Status status, const std::string& response) {
        if (!status)
        {
            cb(status, ActionResult());
            return;
        }

        ActionResult res;

        try
        {
            auto browseResult = xml::parseBrowseResult(response, res);
        #ifdef DEBUG_CONTENT_BROWSING
            log::debug(browseResult);
        #endif

            if (type == ContainersOnly || type == All)
            {
                try { res.result = xml::parseContainers(browseResult); }
                catch (std::exception&e ) { log::warn(e.what()); }
            }

            if (type == ItemsOnly || type == All)
            {
                try
                {
                    auto items = xml::parseItems(browseResult);
                    for (auto& item : items)
                    {
                        res.result.emplace_back(std::move(item));
                    }
                }
                catch (std::exception& e) { log::warn(e.what()); }
            }
        }
        catch (std::exception& e)
        {
            status = Status(ErrorCode::Unexpected, e.what());
        }

        cb(status, res);
    });
}

void Client::search(const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(Status, ActionResult)> cb)
{
    m_abort = false;

    executeAction(Action::Search, { {"ObjectID", objectId},
                                    {"SearchCriteria", criteria},
                                    {"Filter", filter},
                                    {"StartingIndex", numericops::toString(startIndex)},
                                    {"RequestedCount", numericops::toString(limit)},
                                    {"SortCriteria", sort} }, [cb] (Status status, const std::string& response) {
        if (!status)
        {
            cb(status, ActionResult());
            return;
        }

        ActionResult searchResult;

        try
        {
            auto searchResultDoc = xml::parseBrowseResult(response, searchResult);

            try { searchResult.result = xml::parseContainers(searchResultDoc); }
            catch (std::exception&e ) { log::warn(e.what()); }

            try
            {
                auto items = xml::parseItems(searchResultDoc);
                for (auto& item : items)
                {
                    searchResult.result.emplace_back(std::move(item));
                }
            }
            catch (std::exception& e) { log::warn(e.what()); }
        }
        catch (std::exception& e)
        {
            status = Status(ErrorCode::Unexpected, e.what());
        }

        cb(status, searchResult);
    });
}

void Client::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, std::function<void(Status, std::string)> cb)
{
    m_abort = false;

#ifdef DEBUG_CONTENT_BROWSING
    log::debug("Browse: {} {} {} {} {} {}", objectId, flag, filter, startIndex, limit, sort);
#endif

    executeAction(Action::Browse, { {"ObjectID", objectId},
                                    {"BrowseFlag", flag},
                                    {"Filter", filter},
                                    {"StartingIndex", numericops::toString(startIndex)},
                                    {"RequestedCount", numericops::toString(limit)},
                                    {"SortCriteria", sort} }, cb);
}

Future<std::vector<Property>> Client::querySearchCapabilities()
{
    auto response = co_await executeAction(Action::GetSearchCapabilities);
    co_return parseCapabilities("SearchCaps", response);
}

Future<std::vector<Property>> Client::querySortCapabilities()
{
    auto response = co_await executeAction(Action::GetSortCapabilities);
    co_return parseCapabilities("SortCaps", response);
}

std::vector<Property> Client::parseCapabilities(const std::string& nodeName, const std::string& response)
{
    try
    {
        std::vector<Property> props;

        xml_document<> doc;
        doc.parse<parse_non_destructive>(response.c_str());
        auto& caps = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref(nodeName.c_str());

        // TODO: don't fail if the search caps is an empty list
        for (auto& cap : stringops::tokenize(caps.value_string(), ','))
        {
            addPropertyToList(cap, props);
        }

        return props;
    }
    catch (std::exception& e)
    {
        throw Status(ErrorCode::Unexpected, e.what());
    }
}

Future<std::string> Client::querySystemUpdateID()
{
    auto response = co_await executeAction(Action::GetSystemUpdateID);
    std::string sysUpdateId;

    try
    {
        xml_document<> doc;
        doc.parse<parse_non_destructive>(&response.front());
        sysUpdateId = doc.first_node_ref().first_node_ref("Id").value_string();
    }
    catch (std::exception& e)
    {
        throw Status(ErrorCode::Unexpected, e.what());
    }

    co_return sysUpdateId;
}

Future<Item> Client::browseMetadata(const std::string& objectId, const std::string& filter)
{
    auto response = co_await browseAction(objectId, "BrowseMetadata", filter, 0, 0, "");

    ActionResult res;
    auto browseResult = xml::parseBrowseResult(response, res);
    if (browseResult.empty())
    {
        throw Status(ErrorCode::Unexpected, "Failed to browse metadata");
    }

    #ifdef DEBUG_CONTENT_BROWSING
        log::debug(browseResult);
    #endif

    co_return xml::parseMetaData(browseResult);
}

Future<ActionResult> Client::browseDirectChildren(BrowseType type, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    auto response = co_await browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort);
    ActionResult actionResult;

    try
    {
        auto browseResult = xml::parseBrowseResult(response, actionResult);
    #ifdef DEBUG_CONTENT_BROWSING
        log::debug(browseResult);
    #endif

        if (type == ContainersOnly || type == All)
        {
            try { actionResult.result = xml::parseContainers(browseResult); }
            catch (std::exception&e ) { log::warn(e.what()); }
        }

        if (type == ItemsOnly || type == All)
        {
            try
            {
                auto items = xml::parseItems(browseResult);
                for (auto& item : items)
                {
                    actionResult.result.emplace_back(std::move(item));
                }
            }
            catch (std::exception& e) { log::warn(e.what()); }
        }
    }
    catch (std::exception& e)
    {
        throw Status(ErrorCode::Unexpected, e.what());
    }

    co_return actionResult;
}

Future<ActionResult> Client::search(const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_abort = false;

    auto response = co_await executeAction(Action::Search, { {"ObjectID", objectId},
                                                             {"SearchCriteria", criteria},
                                                             {"Filter", filter},
                                                             {"StartingIndex", numericops::toString(startIndex)},
                                                             {"RequestedCount", numericops::toString(limit)},
                                                             {"SortCriteria", sort} });

    ActionResult searchResult;

    try
    {
        auto searchResultDoc = xml::parseBrowseResult(response, searchResult);

        try { searchResult.result = xml::parseContainers(searchResultDoc); }
        catch (std::exception&e ) { log::warn(e.what()); }

        try
        {
            auto items = xml::parseItems(searchResultDoc);
            for (auto& item : items)
            {
                searchResult.result.emplace_back(std::move(item));
            }
        }
        catch (std::exception& e) { log::warn(e.what()); }
    }
    catch (std::exception& e)
    {
        throw Status(ErrorCode::Unexpected, e.what());
    }

    co_return searchResult;
}

Future<std::string> Client::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_abort = false;

#ifdef DEBUG_CONTENT_BROWSING
    log::debug("Browse: {} {} {} {} {} {}", objectId, flag, filter, startIndex, limit, sort);
#endif

    return executeAction(Action::Browse, { {"ObjectID", objectId},
                                           {"BrowseFlag", flag},
                                           {"Filter", filter},
                                           {"StartingIndex", numericops::toString(startIndex)},
                                           {"RequestedCount", numericops::toString(limit)},
                                           {"SortCriteria", sort} });
}

// void Client::handleUPnPResult(int errorCode)
// {
//     if (errorCode == UPNP_E_SUCCESS) return;

//     switch (errorCode)
//     {
//     case 701: throw Exception(errorCode, "No such object, the specified id is invalid");
//     case 702: throw Exception(errorCode, "Invalid CurrentTagValue, probably out of date");
//     case 703: throw Exception(errorCode, "Invalid NewTagValue, parameter is invalid");
//     case 704: throw Exception(errorCode, "Unable to delete a required tag");
//     case 705: throw Exception(errorCode, "UPdate read only tag not allowed");
//     case 706: throw Exception(errorCode, "Parameter Mismatch");
//     case 708: throw Exception(errorCode, "Unsupported or invalid search criteria");
//     case 709: throw Exception(errorCode, "Unsupported or invalid sort criteria");
//     case 710: throw Exception(errorCode, "No such container");
//     case 711: throw Exception(errorCode, "This is a restricted object");
//     case 712: throw Exception(errorCode, "Operation would result in bad metadata");
//     case 713: throw Exception(errorCode, "The parent object is restricted");
//     case 714: throw Exception(errorCode, "No such source resource");
//     case 715: throw Exception(errorCode, "Source resource access denied");
//     case 716: throw Exception(errorCode, "A transfer is busy");
//     case 717: throw Exception(errorCode, "No such file transfer");
//     case 718: throw Exception(errorCode, "No such destination resource");
//     case 719: throw Exception(errorCode, "Destination resource access denied");
//     case 720: throw Exception(errorCode, "Cannot process the request");
//     default: upnp::handleUPnPResult(errorCode);
//     }
// }

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return s_subscriptionTimeout;
}

}
}
