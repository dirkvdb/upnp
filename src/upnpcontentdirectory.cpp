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

//#define DEBUG_CONTENT_BROWSING

namespace upnp
{

static const int32_t g_subscriptionTimeout = 1801;

ContentDirectory::ContentDirectory(IClient& client)
: ServiceBase(client)
, m_Abort(false)
{
    //ixmlRelaxParser(1);
}

void ContentDirectory::setDevice(const std::shared_ptr<Device>& device)
{
    ServiceBase::setDevice(device);

    m_SearchCaps.clear();
    m_SortCaps.clear();
    m_SystemUpdateId.clear();
    
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
    xml::Document result = executeAction(ContentDirectoryAction::GetSearchCapabilities);
    xml::Element elem = result.getFirstChild();
    
    // TODO: don't fail if the search caps is an empty list

    for (auto& cap : stringops::tokenize(elem.getChildNodeValue("SearchCaps"), ","))
    {
        addPropertyToList(cap, m_SearchCaps);
    }
}

void ContentDirectory::querySortCapabilities()
{
    xml::Document result = executeAction(ContentDirectoryAction::GetSortCapabilities);
    xml::Element elem = result.getFirstChild();
    
    // TODO: don't fail if the sort caps is an empty list
    
    for (auto& cap : stringops::tokenize(elem.getChildNodeValue("SortCaps"), ","))
    {
        log::debug(cap);
        addPropertyToList(cap, m_SortCaps);
    }
}

void ContentDirectory::querySystemUpdateID()
{
    xml::Document result = executeAction(ContentDirectoryAction::GetSystemUpdateID);
    xml::Element elem = result.getFirstChild();
    m_SystemUpdateId = elem.getChildNodeValue("Id");
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
    log::debug(browseResult.toString());
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
    
    xml::Document result = executeAction(ContentDirectoryAction::Search, { {"ObjectID", objectId},
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
    
    return executeAction(ContentDirectoryAction::Browse, { {"ObjectID", objectId},
                                                           {"BrowseFlag", flag},
                                                           {"Filter", filter},
                                                           {"StartingIndex", numericops::toString(startIndex)},
                                                           {"RequestedCount", numericops::toString(limit)},
                                                           {"SortCriteria", sort} });
}

xml::Document ContentDirectory::parseBrowseResult(xml::Document& doc, ActionResult& result)
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
    item->setChildCount(containerElem.getAttributeAsNumericOptional<uint32_t>("childCount", 0));
    
    for (xml::Element elem : containerElem.getChildNodes())
    {
        Property prop = propertyFromString(elem.getName());
        if (prop == Property::Unknown)
        {
            log::warn("Unknown property", elem.getName());
            continue;
        }
        
        item->addMetaData(prop, elem.getValue());
    }
    
    // check required properties
    if (item->getTitle().empty())
    {
        throw std::logic_error("No title found in item");
    }
}

Resource ContentDirectory::parseResource(xml::NamedNodeMap& nodeMap, const std::string& url)
{
    Resource res;
    res.setUrl(url);
    
    for (auto& node : nodeMap)
    {
        try
        {
            std::string key = node.getName();
            std::string value = node.getValue();
            
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

    try
    {
        for (xml::Element elem : itemElem.getChildNodes())
        {
            try
            {
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
    for (xml::Element elem : doc.getElementsByTagName("container"))
    {
        try
        {
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
    for (xml::Element elem : doc.getElementsByTagName("item"))
    {
        try
        {
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

ContentDirectoryAction ContentDirectory::actionFromString(const std::string& action)
{
    if (action == "GetSearchCapabilities")  return ContentDirectoryAction::GetSearchCapabilities;
    if (action == "GetSortCapabilities")    return ContentDirectoryAction::GetSortCapabilities;
    if (action == "GetSystemUpdateID")      return ContentDirectoryAction::GetSystemUpdateID;
    if (action == "Browse")                 return ContentDirectoryAction::Browse;
    if (action == "Search")                 return ContentDirectoryAction::Search;
    
    throw std::logic_error("Unknown ContentDirectory action:" + action);
}

std::string ContentDirectory::actionToString(ContentDirectoryAction action)
{
    switch (action)
    {
        case ContentDirectoryAction::GetSearchCapabilities:     return "GetSearchCapabilities";
        case ContentDirectoryAction::GetSortCapabilities:       return "GetSortCapabilities";
        case ContentDirectoryAction::GetSystemUpdateID:         return "GetSystemUpdateID";
        case ContentDirectoryAction::Browse:                    return "Browse";
        case ContentDirectoryAction::Search:                    return "Search";
            
        default:
            throw std::logic_error("Unknown ContentDirectory action");
    }
}

ContentDirectoryVariable ContentDirectory::variableFromString(const std::string& var)
{
    if (var == "ContainerUpdateIDs")                return ContentDirectoryVariable::ContainerUpdateIDs;
    if (var == "TransferIDs")                       return ContentDirectoryVariable::TransferIDs;
    if (var == "SystemUpdateID")                    return ContentDirectoryVariable::SystemUpdateID;
    if (var == "A_ARG_TYPE_ObjectID")               return ContentDirectoryVariable::ArgumentTypeObjectID;
    if (var == "A_ARG_TYPE_Result")                 return ContentDirectoryVariable::ArgumentTypeResult;
    if (var == "A_ARG_TYPE_SearchCriteria")         return ContentDirectoryVariable::ArgumentTypeSearchCriteria;
    if (var == "A_ARG_TYPE_Flag")                   return ContentDirectoryVariable::ArgumentTypeBrowseFlag;
    if (var == "A_ARG_TYPE_Filter")                 return ContentDirectoryVariable::ArgumentTypeFilter;
    if (var == "A_ARG_TYPE_SortCriteria")           return ContentDirectoryVariable::ArgumentTypeSortCriteria;
    if (var == "A_ARG_TYPE_Index")                  return ContentDirectoryVariable::ArgumentTypeIndex;
    if (var == "A_ARG_TYPE_Count")                  return ContentDirectoryVariable::ArgumentTypeCount;
    if (var == "A_ARG_TYPE_UpdateID")               return ContentDirectoryVariable::ArgumentTypeUpdateID;
    if (var == "A_ARG_TYPE_SearchCapabilities")     return ContentDirectoryVariable::ArgumentTypeSearchCapabilities;
    if (var == "A_ARG_TYPE_SortCapabilities")       return ContentDirectoryVariable::ArgumentTypeSortCapabilities;

    throw std::logic_error("Unknown ContentDirectory variable:" + var);
}

std::string ContentDirectory::variableToString(ContentDirectoryVariable var)
{
    switch (var)
    {
        case ContentDirectoryVariable::ContainerUpdateIDs:                  return "ContainerUpdateIDs";
        case ContentDirectoryVariable::TransferIDs:                         return "TransferIDs";
        case ContentDirectoryVariable::SystemUpdateID:                      return "SystemUpdateID";
        case ContentDirectoryVariable::ArgumentTypeObjectID:                return "A_ARG_TYPE_ObjectID";
        case ContentDirectoryVariable::ArgumentTypeResult:                  return "A_ARG_TYPE_Result";
        case ContentDirectoryVariable::ArgumentTypeSearchCriteria:          return "A_ARG_TYPE_SearchCriteria";
        case ContentDirectoryVariable::ArgumentTypeBrowseFlag:              return "A_ARG_TYPE_Flag";
        case ContentDirectoryVariable::ArgumentTypeFilter:                  return "A_ARG_TYPE_Filter";
        case ContentDirectoryVariable::ArgumentTypeSortCriteria:            return "A_ARG_TYPE_SortCriteria";
        case ContentDirectoryVariable::ArgumentTypeIndex:                   return "A_ARG_TYPE_Index";
        case ContentDirectoryVariable::ArgumentTypeCount:                   return "A_ARG_TYPE_Count";
        case ContentDirectoryVariable::ArgumentTypeUpdateID:                return "A_ARG_TYPE_UpdateID";
        case ContentDirectoryVariable::ArgumentTypeSearchCapabilities:      return "A_ARG_TYPE_SearchCapabilities";
        case ContentDirectoryVariable::ArgumentTypeSortCapabilities:        return "A_ARG_TYPE_SortCapabilities";
        
        default:
            throw std::logic_error("Unknown ContentDirectory variable");
    }
}

ServiceType ContentDirectory::getType()
{
    return ServiceType::ContentDirectory;
}

int32_t ContentDirectory::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

}

