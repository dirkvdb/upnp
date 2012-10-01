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

#include "upnp/upnpcontentdirectory.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

using namespace utils;

namespace upnp
{

ContentDirectory::ContentDirectory(IClient& client)
: m_Client(client)
, m_Abort(false)
{
    ixmlRelaxParser(1);
}

void ContentDirectory::setDevice(const std::shared_ptr<Device>& device)
{
    m_SearchCaps.clear();
    m_SortCaps.clear();
    m_SystemUpdateId.clear();
    m_Service = device->m_Services[ServiceType::ContentDirectory];
    
    try { querySearchCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain search capabilities:", e.what()); }
    
    try { querySortCapabilities(); }
    catch (std::exception& e) { log::error("Failed to obtain sort capabilities:", e.what()); }
    
    try { querySystemUpdateID(); }
    catch (std::exception& e) { log::error("Failed to obtain system update id:", e.what()); }
}

void ContentDirectory::abort()
{
    m_Abort = true;
}

const std::vector<Property>& ContentDirectory::getSearchCapabilities() const
{
    return m_SearchCaps;
}

const std::vector<Property>& ContentDirectory::getSortCapabilities() const
{
    return m_SortCaps;
}

void ContentDirectory::querySearchCapabilities()
{
    Action action("GetSearchCapabilities", m_Service.m_ControlURL, ServiceType::ContentDirectory);
    xml::Document result = sendAction(action);
    auto caps = stringops::tokenize(result.getChildElementValueRecursive("SearchCaps"), ",");
    
    for (auto& cap : caps)
    {
        addPropertyToList(cap, m_SearchCaps);
    }
}

void ContentDirectory::querySortCapabilities()
{
    Action action("GetSortCapabilities", m_Service.m_ControlURL, ServiceType::ContentDirectory);
    xml::Document result = sendAction(action);
    auto caps = stringops::tokenize(result.getChildElementValueRecursive("SortCaps"), ",");
    
    for (auto& cap : caps)
    {
        addPropertyToList(cap, m_SortCaps);
    }
}

void ContentDirectory::querySystemUpdateID()
{
    Action action("GetSystemUpdateID", m_Service.m_ControlURL, ServiceType::ContentDirectory);
    xml::Document result = sendAction(action);
    
    m_SystemUpdateId = result.getChildElementValueRecursive("Id");
}

void ContentDirectory::browseMetadata(const std::shared_ptr<Item>& item, const std::string& filter)
{
    ActionResult res;

    xml::Document result = browseAction(item->getObjectId(), "BrowseMetadata", filter, 0, 0, "");
    xml::Document browseResult = parseBrowseResult(result, res);
    if (!browseResult)
    {
        throw std::logic_error("Failed to browse meta data");
    }
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug(IXmlString(xml::DocumenttoString(browseResult)));
#endif
    
    parseMetaData(browseResult, item);
}


ContentDirectory::ActionResult ContentDirectory::browseDirectChildren(BrowseType type, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
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
            notifySubscriber(containers, subscriber);
        }
        catch (std::exception&e ) { log::warn(e.what()); }
    }
    
    if (type == ItemsOnly || type == All)
    {
        try
        {
            auto items = parseItems(browseResult);
            notifySubscriber(items, subscriber);
        }
        catch (std::exception& e) { log::warn(e.what()); }
    }
    
    return res;
}

ContentDirectory::ActionResult ContentDirectory::search(utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
    Action action("Search", m_Service.m_ControlURL, ServiceType::ContentDirectory);
    action.addArgument("ObjectID", objectId);
    action.addArgument("SearchCriteria", criteria);
    action.addArgument("Filter", filter);
    action.addArgument("StartingIndex", numericops::toString(startIndex));
    action.addArgument("RequestedCount", numericops::toString(limit));
    action.addArgument("SortCriteria", sort);
    
    xml::Document result = m_Client.sendAction(action);
    
    ActionResult searchResult;
    xml::Document searchResultDoc = parseBrowseResult(result, searchResult);
    if (!searchResultDoc)
    {
        throw std::logic_error("Failed to perform search");
    }

    try
    {
        auto containers = parseContainers(searchResultDoc);
        notifySubscriber(containers, subscriber);
    }
    catch (std::exception&e ) { log::warn(e.what()); }
    
    try
    {
        auto items = parseItems(searchResultDoc);
        notifySubscriber(items, subscriber);
    }
    catch (std::exception& e) { log::warn(e.what()); }

    return searchResult;
}

void ContentDirectory::notifySubscriber(std::vector<std::shared_ptr<Item>>& items, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    for (auto& item : items)
    {
        if (m_Abort) break;
        
#ifdef DEBUG_CONTENT_BROWSING
        log::debug("Item:", item->getTitle());
#endif

        subscriber.onItem(item);
    }
}

xml::Document ContentDirectory::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug("Browse:", objectId, flag, filter, startIndex, limit, sort);
#endif
    
    Action browseAction("Browse", m_Service.m_ControlURL, ServiceType::ContentDirectory);
    browseAction.addArgument("ObjectID", objectId);
    browseAction.addArgument("BrowseFlag", flag);
    browseAction.addArgument("Filter", filter);
    browseAction.addArgument("StartingIndex", numericops::toString(startIndex));
    browseAction.addArgument("RequestedCount", numericops::toString(limit));
    browseAction.addArgument("SortCriteria", sort);
    
    try
    {
        return m_Client.sendAction(browseAction);
    }
    catch (UPnPException& e)
    {
        handleUPnPResult(e.getErrorCode());
    }
    
    assert(false);
    return xml::Document();
}

xml::Document ContentDirectory::parseBrowseResult(xml::Document& doc, ActionResult& result)
{
    assert(doc && "ParseBrowseResult: Invalid document supplied");
    
    xml::Document browseDoc(doc.getChildElementValueRecursive("Result"));
    result.numberReturned = stringops::toNumeric<uint32_t>(doc.getChildElementValueRecursive("NumberReturned"));
    result.totalMatches = stringops::toNumeric<uint32_t>(doc.getChildElementValueRecursive("TotalMatches"));
    
    return browseDoc;
}

void ContentDirectory::parseMetaData(xml::Document& doc, const std::shared_ptr<Item>& item)
{
    assert(doc && "ParseMetaData: Invalid document supplied");
    
    try
    {
        xml::Element containerElem = doc.getElementsByTagName("container").getNode(0);
        return parseContainer(containerElem, item);
    }
    catch (std::exception&) {}
    
    try
    {
        xml::Element itemElem = doc.getElementsByTagName("item").getNode(0);
        return parseItem(itemElem, item);
    }
    catch (std::exception&) {}
    
    log::warn("No metadata could be retrieved");
}

void ContentDirectory::parseContainer(xml::Element& containerElem, const std::shared_ptr<Item>& item)
{
    item->setObjectId(containerElem.getAttribute("id"));
    item->setParentId(containerElem.getAttribute("parentID"));
    
    try
    {
        item->setChildCount(stringops::toNumeric<uint32_t>(containerElem.getAttribute("childCount")));
    }
    catch (std::exception&) { /* Childcount attribute is not obligated */ }
    
    // required properties
    item->setTitle(containerElem.getChildElementValue("dc:title"));
    item->addMetaData(Property::Class, containerElem.getChildElementValue("upnp:class"));
    
    // optional properties
    try
    {
        item->addMetaData(Property::AlbumArt, containerElem.getChildElementValue("upnp:albumArtURI"));
    }
    catch (std::exception&) {}
    
    try
    {
        item->addMetaData(Property::Artist, containerElem.getChildElementValue("upnp:artist"));
    }
    catch (std::exception&) {}
}

Resource ContentDirectory::parseResource(xml::NamedNodeMap& nodeMap, const std::string& url)
{
    Resource res;
    res.setUrl(url);
    
    uint64_t numAttributes = nodeMap.size();
    for (uint64_t i = 0; i < numAttributes && !m_Abort; ++i)
    {
        try
        {
            xml::Node item = nodeMap.getNode(i);
            std::string key = item.getName();
            std::string value = item.getValue();
            
            if (key == "protocolInfo")
            {
                res.setProtocolInfo(ProtocolInfo(value));
            }
            else
            {
                res.addMetaData(key, value);
            }
        }
        catch (std::exception& e) { /* skip invalid resource */ log::warn(e.what()); }
    }
    
    return res;
}

void ContentDirectory::parseItem(xml::Element& itemElem, const std::shared_ptr<Item>& item)
{
    item->setObjectId(itemElem.getAttribute("id"));
    item->setParentId(itemElem.getAttribute("parentID"));
    item->setTitle(itemElem.getChildElementValue("dc:title"));

    try
    {
        xml::NodeList children = itemElem.getChildNodes();
        uint64_t numChildren = children.size();
        for (uint64_t i = 0; i < numChildren && !m_Abort; ++i)
        {
            try
            {
                xml::Element elem = children.getNode(i);
                std::string key     = elem.getName();
                std::string value   = elem.getValue();
                
                if ("res" == key)
                {
                    auto nodeMap = elem.getAttributes();
                    item->addResource(parseResource(nodeMap, value));
                }
                else
                {
                    addPropertyToItem(key, value, item);
                }
            }
            catch (std::exception& e) { /* try to parse the rest */ log::warn("Failed to parse upnp item:", e.what()); }
        }
        
#ifdef DEBUG_CONTENT_BROWSING
        log::debug(*item);
#endif
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item");
    }
}

std::vector<std::shared_ptr<Item>> ContentDirectory::parseContainers(xml::Document& doc)
{
    assert(doc && "ParseContainers: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> containers;
    xml::NodeList containerList = doc.getElementsByTagName("container");
    uint64_t numContainers = containerList.size();
    containers.reserve(numContainers);
    
    for (uint64_t i = 0; i < numContainers && !m_Abort; ++i)
    {
        try
        {
            xml::Element elem = containerList.getNode(i);
            
            auto item = std::make_shared<Item>();
            parseContainer(elem, item);
            containers.push_back(item);
        }
        catch (std::exception& e)
        {
            log::warn(std::string("Failed to parse container, skipping (") + e.what() + ")");
        }
    }
    
    return containers;
}

std::vector<std::shared_ptr<Item>> ContentDirectory::parseItems(xml::Document& doc)
{
    assert(doc && "ParseItems: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> items;
    xml::NodeList itemList = doc.getElementsByTagName("item");
    uint64_t numItems = itemList.size();
    for (uint64_t i = 0; i < numItems && !m_Abort; ++i)
    {
        try
        {
            xml::Element elem = itemList.getNode(i);
        
            auto item = std::make_shared<Item>();
            parseItem(elem, item);
            items.push_back(item);
        }
        catch (std::exception& e)
        {
            log::error(std::string("Failed to parse item, skipping (") + e.what() + ")");
        }
    }
    
    return items;
}

xml::Document ContentDirectory::sendAction(const Action& action)
{
    try
    {
        return m_Client.sendAction(action);
    }
    catch (UPnPException& e)
    {
        handleUPnPResult(e.getErrorCode());
    }
    
    assert(false);
    return xml::Document();
}

void ContentDirectory::handleUPnPResult(int errorCode)
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

void ContentDirectory::addPropertyToItem(const std::string& propertyName, const std::string& propertyValue, const std::shared_ptr<Item>& item)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        item->addMetaData(prop, propertyValue);
    }
    else
    {
        log::warn("Unknown property:", propertyName);
    }
}

void ContentDirectory::addPropertyToList(const std::string& propertyName, std::vector<Property>& vec)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        vec.push_back(prop);
    }
    else
    {
        log::warn("Unknown property:", propertyName);
    }
}

}

