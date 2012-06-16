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

#include "upnp/upnpclient.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

using namespace utils;

static const char* ContentDirectoryServiceType = "urn:schemas-upnp-org:service:ContentDirectory:1";


namespace upnp
{

ContentDirectory::ContentDirectory(const Client& client)
: m_Client(client)
, m_Abort(false)
{
    ixmlRelaxParser(1);
}

void ContentDirectory::setDevice(std::shared_ptr<Device> device)
{
    m_SearchCaps.clear();
    m_SortCaps.clear();
    m_SystemUpdateId.clear();
    m_Device = device;
    
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
    Action action("GetSearchCapabilities", ContentDirectoryServiceType);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    auto caps = stringops::tokenize(getFirstElementValue(result, "SearchCaps"), ",");
    
    for (auto& cap : caps)
    {
        addPropertyToList(cap, m_SearchCaps);
    }
}

void ContentDirectory::querySortCapabilities()
{
    Action action("GetSortCapabilities", ContentDirectoryServiceType);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    auto caps = stringops::tokenize(getFirstElementValue(result, "SortCaps"), ",");
    
    for (auto& cap : caps)
    {
        addPropertyToList(cap, m_SortCaps);
    }
}

void ContentDirectory::querySystemUpdateID()
{
    Action action("GetSystemUpdateID", ContentDirectoryServiceType);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    m_SystemUpdateId = getFirstElementValue(result, "Id");
}

void ContentDirectory::browseMetadata(std::shared_ptr<Item>& item, const std::string& filter)
{
    ActionResult res;

    IXmlDocument result = browseAction(item->getObjectId(), "BrowseMetadata", filter, 0, 0, "");
    IXmlDocument browseResult = parseBrowseResult(result, res);
    if (!browseResult)
    {
        throw std::logic_error("Failed to browse meta data");
    }
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug(IXmlString(ixmlDocumenttoString(browseResult)));
#endif
    
    parseMetaData(browseResult, item);
}


ContentDirectory::ActionResult ContentDirectory::browseDirectChildren(BrowseType type, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    ActionResult res;

    IXmlDocument result = browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort);
    IXmlDocument browseResult = parseBrowseResult(result, res);
    if (!browseResult)
    {
        throw std::logic_error("Failed to browse direct children");
    }

#ifdef DEBUG_CONTENT_BROWSING
    log::debug(IXmlString(ixmlDocumenttoString(browseResult)));
#endif

    if (type == ContainersOnly || type == All)
    {
        auto containers = parseContainers(browseResult);
        notifySubscriber(containers, subscriber);
    }
    
    if (type == ItemsOnly || type == All)
    {
        auto items = parseItems(browseResult);
        notifySubscriber(items, subscriber);
    }
    
    return res;
}

ContentDirectory::ActionResult ContentDirectory::search(utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
    Action action("Search", ContentDirectoryServiceType);
    action.addArgument("ObjectID", objectId);
    action.addArgument("SearchCriteria", criteria);
    action.addArgument("Filter", filter);
    action.addArgument("StartingIndex", numericops::toString(startIndex));
    action.addArgument("RequestedCount", numericops::toString(limit));
    action.addArgument("SortCriteria", sort);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    ActionResult searchResult;
    IXmlDocument searchResultDoc = parseBrowseResult(result, searchResult);
    if (!searchResultDoc)
    {
        throw std::logic_error("Failed to perform search");
    }

    auto containers = parseContainers(searchResultDoc);
    notifySubscriber(containers, subscriber);
    auto items = parseItems(searchResultDoc);
    notifySubscriber(items, subscriber);

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

IXML_Document* ContentDirectory::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
#ifdef DEBUG_CONTENT_BROWSING
    log::debug("Browse:", objectId, flag, filter, startIndex, limit, sort);
#endif
    
    Action browseAction("Browse", ContentDirectoryServiceType);
    browseAction.addArgument("ObjectID", objectId);
    browseAction.addArgument("BrowseFlag", flag);
    browseAction.addArgument("Filter", filter);
    browseAction.addArgument("StartingIndex", numericops::toString(startIndex));
    browseAction.addArgument("RequestedCount", numericops::toString(limit));
    browseAction.addArgument("SortCriteria", sort);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, browseAction.getActionDocument(), &pResult));
    
    return pResult;
}

IXmlDocument ContentDirectory::parseBrowseResult(IXmlDocument& doc, ActionResult& result)
{
    assert(doc && "ParseBrowseResult: Invalid document supplied");
    
    std::string xml = getFirstElementValue(doc, "Result");
    IXmlDocument browseDoc = ixmlParseBuffer(xml.c_str());

    result.numberReturned = stringops::toNumeric<uint32_t>(getFirstElementValue(doc, "NumberReturned"));
    result.totalMatches = stringops::toNumeric<uint32_t>(getFirstElementValue(doc, "TotalMatches"));
    
    return browseDoc;
}

void ContentDirectory::parseMetaData(IXmlDocument& doc, std::shared_ptr<Item>& item)
{
    assert(doc && "ParseMetaData: Invalid document supplied");
    
    IXmlNodeList itemList = ixmlDocument_getElementsByTagName(doc, "container");
    if (itemList)
    {
        unsigned long numContainers = ixmlNodeList_length(itemList);
        assert(numContainers == 1);
        
        parseContainer(reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, 0)), item);
        return;
    }
    
    itemList = ixmlDocument_getElementsByTagName(doc, "item");
    if (itemList)
    {
        unsigned long numContainers = ixmlNodeList_length(itemList);
        assert(numContainers == 1);
        
        parseItem(reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, 0)), item);
        return;
    }
    
    log::warn("No metadata could be retrieved");
}

void ContentDirectory::parseContainer(IXML_Element* pContainerElem, std::shared_ptr<Item>& item)
{
    throwOnNull(pContainerElem, "Null container");
    
    const char* pId = ixmlElement_getAttribute(pContainerElem, "id");
    throwOnNull(pId, "No id in container");
    
    const char* pParentId = ixmlElement_getAttribute(pContainerElem, "parentID");
    throwOnNull(pParentId, "No parent id in container");
    
    const char* pChildCount = ixmlElement_getAttribute(pContainerElem, "childCount");
    
    item->setObjectId(pId);
    item->setParentId(pParentId);
    
    if (pChildCount)
    {
        item->setChildCount(stringops::toNumeric<uint32_t>(pChildCount));
    }
    
    item->setTitle(getFirstElementValue(pContainerElem, "dc:title"));
    item->addMetaData(Property::Class, getFirstElementValue(pContainerElem, "upnp:class"));
    item->addMetaData(Property::AlbumArt, getFirstElementValue(pContainerElem, "upnp:albumArtURI"));
    item->addMetaData(Property::Artist, getFirstElementValue(pContainerElem, "upnp:artist"));
}

Resource ContentDirectory::parseResource(IXML_NamedNodeMap* pNodeMap, const char* pUrl)
{
    throwOnNull(pNodeMap, "Null nodemap");
    
    Resource res;
    res.setUrl(pUrl);
    
    unsigned long numAttributes = ixmlNamedNodeMap_getLength(pNodeMap);
    for (unsigned long i = 0; i < numAttributes && !m_Abort; ++i)
    {
        IXML_Node* pItem = ixmlNamedNodeMap_item(pNodeMap, i);
        const char* pKey = ixmlNode_getNodeName(pItem);
        if (!pKey) continue;
        
        const char* pValue = ixmlNode_getNodeValue(pItem);
        if (!pValue) continue;
        
        //log::debug(pKey, "-", pValue);
        if (std::string("protocolInfo") == pKey)
        {
            try
            {
                res.setProtocolInfo(ProtocolInfo(pValue));
            }
            catch (std::exception& e)
            {
                log::warn(e.what());
            }
        }
        else
        {
            res.addMetaData(pKey, pValue);
        }
    }
    
    ixmlNamedNodeMap_free(pNodeMap);
    
    return res;
}

void ContentDirectory::parseItem(IXML_Element* pItemElem, std::shared_ptr<Item>& item)
{
    const char* pId = ixmlElement_getAttribute(pItemElem, "id");
    throwOnNull(pId, "No id in item");
    
    const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
    throwOnNull(pId, "No parent id in item");
    
    try
    {
        item->setObjectId(pId);
        item->setParentId(pParentId);
        item->setTitle(getFirstElementValue(pItemElem, "dc:title"));
        
        IXmlNodeList children = ixmlNode_getChildNodes(reinterpret_cast<IXML_Node*>(pItemElem));
        if (children)
        {
            uint32_t numChildren = ixmlNodeList_length(children);
            for (uint32_t i = 0; i < numChildren && !m_Abort; ++i)
            {
                IXML_Element* pChild = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(children, i));
                const char* pKey = ixmlElement_getTagName(pChild);
                if (!pKey) continue;
                
                IXML_Node* pTextNode = ixmlNode_getFirstChild((IXML_Node*)pChild);
                if (!pTextNode) continue;
                
                const char* pValue = ixmlNode_getNodeValue(pTextNode);
                if (!pValue) continue;
                
                if (std::string("res") == pKey)
                {
                    item->addResource(parseResource(ixmlNode_getAttributes(reinterpret_cast<IXML_Node*>(pChild)), pValue));
                }
                else
                {
                    //log::debug(pKey, "-", pValue);
                    addPropertyToItem(pKey, pValue, item);
                }
            }
        }
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item");
    }
}

std::vector<std::shared_ptr<Item>> ContentDirectory::parseContainers(IXmlDocument& doc)
{
    assert(doc && "ParseContainers: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> containers;
    IXmlNodeList containerList = ixmlDocument_getElementsByTagName(doc, "container");
    if (containerList)
    {
        unsigned long numContainers = ixmlNodeList_length(containerList);
        containers.reserve(numContainers);
        
        for (unsigned long i = 0; i < numContainers && !m_Abort; ++i)
        {
            try
            {
                auto item = std::make_shared<Item>();
                parseContainer(reinterpret_cast<IXML_Element*>(ixmlNodeList_item(containerList, i)), item);
                containers.push_back(item);
            }
            catch (std::exception& e)
            {
                log::error(std::string("Failed to parse container, skipping (") + e.what() + ")");
            }
        }
    }
    
    return containers;
}

std::vector<std::shared_ptr<Item>> ContentDirectory::parseItems(IXmlDocument& doc)
{
    assert(doc && "ParseItems: Invalid document supplied");

    std::vector<std::shared_ptr<Item>> items;
    IXmlNodeList itemList = ixmlDocument_getElementsByTagName(doc, "item");
    if (itemList)
    {
        unsigned long numItems = ixmlNodeList_length(itemList);
        for (unsigned long i = 0; i < numItems && !m_Abort; ++i)
        {
            try
            {
                auto item = std::make_shared<Item>();
                parseItem(reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, i)), item);
                items.push_back(item);
            }
            catch (std::exception& e)
            {
                log::error(std::string("Failed to parse item, skipping (") + e.what() + ")");
            }
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

void ContentDirectory::addPropertyToItem(const char* pPropertyName, const char* pPropertyValue, std::shared_ptr<Item>& item)
{
    Property prop = propertyFromString(pPropertyName);
    if (prop != Property::Unknown)
    {
        item->addMetaData(prop, pPropertyValue);
    }
    else
    {
        log::warn("Unknown property:", pPropertyName);
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

