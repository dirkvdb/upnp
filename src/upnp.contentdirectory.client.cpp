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
#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnputils.h"

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

const std::chrono::seconds g_subscriptionTimeout(1801);

void addPropertyToList(const std::string& propertyName, std::vector<Property>& vec)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        vec.push_back(prop);
    }
    else
    {
        log::warn("Unknown property: {}", propertyName);
    }
}

}

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

Client::Client(upnp::IClient2& client)
: ServiceClientBase(client)
, m_abort(false)
{
    ixmlRelaxParser(1);
}

void Client::setDevice(const std::shared_ptr<Device>& device)
{
    ServiceClientBase::setDevice(device);

    m_searchCaps.clear();
    m_sortCaps.clear();
    m_systemUpdateId.clear();

    try { querySearchCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain search capabilities: {}", e.what()); }

    try { querySortCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain sort capabilities: {}", e.what()); }

    try { querySystemUpdateID(); }
    catch (std::exception& e) { log::error("Failed to obtain system update id: {}", e.what()); }
}

void Client::abort()
{
    m_abort = true;
}

const std::vector<Property>& Client::getSearchCapabilities() const
{
    return m_searchCaps;
}

const std::vector<Property>& Client::getSortCapabilities() const
{
    return m_sortCaps;
}

void Client::querySearchCapabilities()
{
    executeAction(Action::GetSearchCapabilities, [this] (int32_t status, const std::string& response) {
        if (status != 200)
        {
            return;
        }

        try
        {
            xml_document<> doc;
            doc.parse<parse_non_destructive>(response.c_str());
            auto& caps = doc.first_node_ref().first_node_ref("SearchCaps");

            // TODO: don't fail if the search caps is an empty list
            for (auto& cap : stringops::tokenize(caps.value_string(), ','))
            {
                addPropertyToList(cap, m_searchCaps);
            }
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    });
}

void Client::querySortCapabilities()
{
    executeAction(Action::GetSortCapabilities, [this] (int32_t status, const std::string& response) {
        if (status != 200)
        {
            return;
        }

        try
        {
            xml_document<> doc;
            doc.parse<parse_non_destructive>(&response.front());
            auto& caps = doc.first_node_ref().first_node_ref("SortCaps");

            // TODO: don't fail if the search caps is an empty list
            for (auto& cap : stringops::tokenize(caps.value_string(), ','))
            {
                addPropertyToList(cap, m_sortCaps);
            }
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    });
}

void Client::querySystemUpdateID()
{
    executeAction(Action::GetSystemUpdateID, [this] (int32_t status, const std::string& response) {
        if (status != 200)
        {
            return;
        }

        try
        {
            xml_document<> doc;
            doc.parse<parse_non_destructive>(&response.front());
            m_systemUpdateId = doc.first_node_ref().first_node_ref("Id").value_string();
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    });
}

void Client::browseMetadata(const std::string& objectId, const std::string& filter, const std::function<void(int32_t, Item)> cb)
{
    browseAction(objectId, "BrowseMetadata", filter, 0, 0, "", [this, cb] (int32_t status, const std::string& response) {
        if (status != 200)
        {
            cb(status, Item());
            return;
        }

        ActionResult res;
        auto browseResult = xml::parseBrowseResult(response, res);
        if (browseResult.empty())
        {
            throw Exception("Failed to browse meta data");
        }

        #ifdef DEBUG_CONTENT_BROWSING
            log::debug(browseResult);
        #endif

        cb(status, xml::parseMetaData(browseResult));
    });
}

void Client::browseDirectChildren(BrowseType type, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb)
{
    browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort, [type, cb] (int32_t status, const std::string& response) {
        if (status != 200)
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
            log::error(e.what());
            status = -1;
        }

        cb(status, res);
    });
}

void Client::search(const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb)
{
    m_abort = false;

    executeAction(Action::Search, { {"ObjectID", objectId},
                                    {"SearchCriteria", criteria},
                                    {"Filter", filter},
                                    {"StartingIndex", numericops::toString(startIndex)},
                                    {"RequestedCount", numericops::toString(limit)},
                                    {"SortCriteria", sort} }, [cb] (int32_t status, const std::string& response) {
        if (status != 200)
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
            log::error(e.what());
            status = -1;
        }

        cb(status, searchResult);
    });
}

void Client::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, std::function<void(int32_t, std::string)> cb)
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

void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;

    switch (errorCode)
    {
    case 701: throw Exception(errorCode, "No such object, the specified id is invalid");
    case 702: throw Exception(errorCode, "Invalid CurrentTagValue, probably out of date");
    case 703: throw Exception(errorCode, "Invalid NewTagValue, parameter is invalid");
    case 704: throw Exception(errorCode, "Unable to delete a required tag");
    case 705: throw Exception(errorCode, "UPdate read only tag not allowed");
    case 706: throw Exception(errorCode, "Parameter Mismatch");
    case 708: throw Exception(errorCode, "Unsupported or invalid search criteria");
    case 709: throw Exception(errorCode, "Unsupported or invalid sort criteria");
    case 710: throw Exception(errorCode, "No such container");
    case 711: throw Exception(errorCode, "This is a restricted object");
    case 712: throw Exception(errorCode, "Operation would result in bad metadata");
    case 713: throw Exception(errorCode, "The parent object is restricted");
    case 714: throw Exception(errorCode, "No such source resource");
    case 715: throw Exception(errorCode, "Source resource access denied");
    case 716: throw Exception(errorCode, "A transfer is busy");
    case 717: throw Exception(errorCode, "No such file transfer");
    case 718: throw Exception(errorCode, "No such destination resource");
    case 719: throw Exception(errorCode, "Destination resource access denied");
    case 720: throw Exception(errorCode, "Cannot process the request");
    default: upnp::handleUPnPResult(errorCode);
    }
}

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

}
}
