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

#include "upnp/upnpcontentdirectoryclient.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

using namespace utils;

//#define DEBUG_CONTENT_BROWSING

namespace upnp
{
namespace ContentDirectory
{

static const int32_t g_subscriptionTimeout = 1801;

Client::Client(IClient& client)
: ServiceBase(client)
, m_Abort(false)
{
    ixmlRelaxParser(1);
}

void Client::setDevice(const std::shared_ptr<Device>& device)
{
    ServiceBase::setDevice(device);

    m_SearchCaps.clear();
    m_SortCaps.clear();
    m_SystemUpdateId.clear();
    
    try { querySearchCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain search capabilities: %s", e.what()); }
    
    try { querySortCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain sort capabilities: %s", e.what()); }
    
    try { querySystemUpdateID(); }
    catch (std::exception& e) { log::error("Failed to obtain system update id: %s", e.what()); }
}

void Client::abort()
{
    m_Abort = true;
}

const std::vector<Property>& Client::getSearchCapabilities() const
{
    return m_SearchCaps;
}

const std::vector<Property>& Client::getSortCapabilities() const
{
    return m_SortCaps;
}

void Client::querySearchCapabilities()
{
    xml::Document result = executeAction(Action::GetSearchCapabilities);
    xml::Element elem = result.getFirstChild();
    
    // TODO: don't fail if the search caps is an empty list

    for (auto& cap : stringops::tokenize(elem.getChildNodeValue("SearchCaps"), ","))
    {
        addPropertyToList(cap, m_SearchCaps);
    }
}

void Client::querySortCapabilities()
{
    xml::Document result = executeAction(Action::GetSortCapabilities);
    xml::Element elem = result.getFirstChild();
    
    // TODO: don't fail if the sort caps is an empty list
    
    for (auto& cap : stringops::tokenize(elem.getChildNodeValue("SortCaps"), ","))
    {
        addPropertyToList(cap, m_SortCaps);
    }
}

void Client::querySystemUpdateID()
{
    xml::Document result = executeAction(Action::GetSystemUpdateID);
    xml::Element elem = result.getFirstChild();
    m_SystemUpdateId = elem.getChildNodeValue("Id");
}

ItemPtr Client::browseMetadata(const std::string& objectId, const std::string& filter)
{
    ActionResult res;

    xml::Document result = browseAction(objectId, "BrowseMetadata", filter, 0, 0, "");
    xml::Document browseResult = parseBrowseResult(result, res);
    if (!browseResult)
    {
        throw std::logic_error("Failed to browse meta data");
    }
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug(browseResult.toString());
#endif
    
    return parseMetaData(browseResult);
}


Client::ActionResult Client::browseDirectChildren(BrowseType type, const ItemCb& onItem, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    ActionResult res;

    xml::Document result = browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort);
    xml::Document browseResult = parseBrowseResult(result, res);
    if (!browseResult)
    {
        throw std::logic_error("Failed to browse direct children");
    }

#ifdef DEBUG_CONTENT_BROWSING
    log::debug(browseResult.toString());
#endif

    if (type == ContainersOnly || type == All)
    {
        try
        {
            auto containers = parseContainers(browseResult);
            notifySubscriber(containers, onItem);
        }
        catch (std::exception&e ) { log::warn(e.what()); }
    }
    
    if (type == ItemsOnly || type == All)
    {
        try
        {
            auto items = parseItems(browseResult);
            notifySubscriber(items, onItem);
        }
        catch (std::exception& e) { log::warn(e.what()); }
    }
    
    return res;
}

Client::ActionResult Client::search(const ItemCb& onItem, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
    xml::Document result = executeAction(Action::Search, { {"ObjectID", objectId},
                                                           {"SearchCriteria", criteria},
                                                           {"Filter", filter},
                                                           {"StartingIndex", numericops::toString(startIndex)},
                                                           {"RequestedCount", numericops::toString(limit)},
                                                           {"SortCriteria", sort} });
    
    ActionResult searchResult;
    xml::Document searchResultDoc = parseBrowseResult(result, searchResult);
    if (!searchResultDoc)
    {
        throw std::logic_error("Failed to perform search");
    }

    try
    {
        auto containers = parseContainers(searchResultDoc);
        notifySubscriber(containers, onItem);
    }
    catch (std::exception&e ) { log::warn(e.what()); }
    
    try
    {
        auto items = parseItems(searchResultDoc);
        notifySubscriber(items, onItem);
    }
    catch (std::exception& e) { log::warn(e.what()); }

    return searchResult;
}

void Client::notifySubscriber(std::vector<std::shared_ptr<Item>>& items, const ItemCb& onItem)
{
    for (auto& item : items)
    {
        if (m_Abort) break;
        
#ifdef DEBUG_CONTENT_BROWSING
        log::debug("Item: %s", item->getTitle());
#endif

        onItem(item);
    }
}

xml::Document Client::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug("Browse: %s %s %s %d %d %s", objectId, flag, filter, startIndex, limit, sort);
#endif
    
    return executeAction(Action::Browse, { {"ObjectID", objectId},
                                           {"BrowseFlag", flag},
                                           {"Filter", filter},
                                           {"StartingIndex", numericops::toString(startIndex)},
                                           {"RequestedCount", numericops::toString(limit)},
                                           {"SortCriteria", sort} });
}

xml::Document Client::parseBrowseResult(xml::Document& doc, ActionResult& result)
{
    std::string browseResult;

    assert(doc && "ParseBrowseResult: Invalid document supplied");
    
    for (xml::Element elem : doc.getFirstChild().getChildNodes())
    {
        if (elem.getName() == "Result")
        {
            browseResult = elem.getValue();
        }
        else if (elem.getName() == "NumberReturned")
        {
            result.numberReturned = stringops::toNumeric<uint32_t>(elem.getValue());
        }
        else if (elem.getName() == "TotalMatches")
        {
            result.totalMatches = stringops::toNumeric<uint32_t>(elem.getValue());
        }
    }
    
    if (browseResult.empty())
    {
        throw std::logic_error("Failed to obtain browse result");
    }
    
    return xml::Document(browseResult);
}

ItemPtr Client::parseMetaData(xml::Document& doc)
{
    assert(doc && "ParseMetaData: Invalid document supplied");
    
    try
    {
        xml::Element containerElem = doc.getElementsByTagName("container").getNode(0);
        return parseContainer(containerElem);
    }
    catch (std::exception&) {}
    
    try
    {
        xml::Element itemElem = doc.getElementsByTagName("item").getNode(0);
        return xml::utils::parseItem(itemElem);
    }
    catch (std::exception&) {}
    
    log::warn("No metadata could be retrieved");
    return ItemPtr();
}

ItemPtr Client::parseContainer(xml::Element& containerElem)
{
    auto item = std::make_shared<Item>();
    item->setObjectId(containerElem.getAttribute("id"));
    item->setParentId(containerElem.getAttribute("parentID"));
    item->setChildCount(containerElem.getAttributeAsNumericOptional<uint32_t>("childCount", 0));
    
    for (xml::Element elem : containerElem.getChildNodes())
    {
        Property prop = propertyFromString(elem.getName());
        if (prop == Property::Unknown)
        {
            log::warn("Unknown property %s", elem.getName());
            continue;
        }
        
        item->addMetaData(prop, elem.getValue());
    }
    
    // check required properties
    if (item->getTitle().empty())
    {
        throw std::logic_error("No title found in item");
    }
    
    return item;
}



std::vector<ItemPtr> Client::parseContainers(xml::Document& doc)
{
    assert(doc && "ParseContainers: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> containers;
    for (xml::Element elem : doc.getElementsByTagName("container"))
    {
        try
        {
            containers.push_back(parseContainer(elem));
        }
        catch (std::exception& e)
        {
            log::warn("Failed to parse container, skipping (%s)", e.what());
        }
    }
    
    return containers;
}

std::vector<ItemPtr> Client::parseItems(xml::Document& doc)
{
    assert(doc && "ParseItems: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> items;
    for (xml::Element elem : doc.getElementsByTagName("item"))
    {
        try
        {
            items.push_back(xml::utils::parseItem(elem));
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse item, skipping (%s)", e.what());
        }
    }
    
    return items;
}

void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;

    switch (errorCode)
    {
    case 701: throw std::logic_error("No such object, the specified id is invalid");
    case 702: throw std::logic_error("Invalid CurrentTagValue, probably out of date");
    case 703: throw std::logic_error("Invalid NewTagValue, parameter is invalid");
    case 704: throw std::logic_error("Unable to delete a required tag");
    case 705: throw std::logic_error("UPdate read only tag not allowed");
    case 706: throw std::logic_error("Parameter Mismatch");
    case 708: throw std::logic_error("Unsupported or invalid search criteria");
    case 709: throw std::logic_error("Unsupported or invalid sort criteria");
    case 710: throw std::logic_error("No such container");
    case 711: throw std::logic_error("This is a restricted object");
    case 712: throw std::logic_error("Operation would result in bad metadata");
    case 713: throw std::logic_error("The parent object is restricted");
    case 714: throw std::logic_error("No such source resource");
    case 715: throw std::logic_error("Source resource access denied");
    case 716: throw std::logic_error("A transfer is busy");
    case 717: throw std::logic_error("No such file transfer");
    case 718: throw std::logic_error("No such destination resource");
    case 719: throw std::logic_error("Destination resource access denied");
    case 720: throw std::logic_error("Cannot process the request");
    default: upnp::handleUPnPResult(errorCode);
    }
}

void Client::addPropertyToList(const std::string& propertyName, std::vector<Property>& vec)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        vec.push_back(prop);
    }
    else
    {
        log::warn("Unknown property: %s", propertyName);
    }
}

Action Client::actionFromString(const std::string& action) const
{
    return ContentDirectory::actionFromString(action);
}

std::string Client::actionToString(Action action) const
{
    return ContentDirectory::actionToString(action);
}

Variable Client::variableFromString(const std::string& var) const
{
    return ContentDirectory::variableFromString(var);
}

std::string Client::variableToString(Variable var) const 
{
    return ContentDirectory::variableToString(var);
}

ServiceType Client::getType()
{
    return ServiceType::ContentDirectory;
}

int32_t Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

}
}
