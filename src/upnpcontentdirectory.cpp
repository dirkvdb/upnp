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
#include "upnp/upnpxmlutils.h"

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
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &pResult));
    
    auto caps = stringops::tokenize(getFirstElementValue(pResult, "SearchCaps"), ",");
    
    for (auto& cap : caps)
    {
        Property prop = propertyFromString(cap);
        if (prop != Property::Unknown)
        {
            m_SearchCaps.push_back(prop);
        }
        else
        {
            log::warn("Unknown search capability", cap);
        }
    }
}

void ContentDirectory::querySortCapabilities()
{
    Action action("GetSortCapabilities", ContentDirectoryServiceType);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &pResult));
    
    auto caps = stringops::tokenize(getFirstElementValue(pResult, "SortCaps"), ",");
    
    for (auto& cap : caps)
    {
        Property prop = propertyFromString(cap);
        if (prop != Property::Unknown)
        {
            m_SortCaps.push_back(prop);
        }
        else
        {
            log::warn("Unknown sort capability", cap);
        }
    }
}

void ContentDirectory::querySystemUpdateID()
{
    Action action("GetSystemUpdateID", ContentDirectoryServiceType);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &pResult));
    
    m_SystemUpdateId = getFirstElementValue(pResult, "Id");
    
    log::debug("SystemUpdateId", m_SystemUpdateId);
    
}

void ContentDirectory::browseMetadata(Item& item, const std::string& filter)
{
    SearchResult res;

    IXML_Document* pResult = browseAction(item.getObjectId(), "BrowseMetadata", filter, 0, 0, "");
    IXML_Document* pBrowseResult = parseBrowseResult(pResult, res);
    if (!pBrowseResult)
    {
        throw std::logic_error("Failed to browse meta data");
    }
    
    parseMetaData(pBrowseResult, item);
    
    ixmlDocument_free(pBrowseResult);
    ixmlDocument_free(pResult);
}


void ContentDirectory::browseDirectChildren(BrowseType type, utils::ISubscriber<Item>& subscriber, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    SearchResult res;

    IXML_Document* pResult = browseAction(objectId, "BrowseDirectChildren", filter, startIndex, limit, sort);
    IXML_Document* pBrowseResult = parseBrowseResult(pResult, res);
    if (!pBrowseResult)
    {
        ixmlDocument_free(pResult);
        throw std::logic_error("Failed to browse direct children");
    }
    
    if (type == ContainersOnly || type == All)
    {
        notifySubscriber(parseContainers(pBrowseResult), subscriber);
    }
    
    if (type == ItemsOnly || type == All)
    {
        notifySubscriber(parseItems(pBrowseResult), subscriber);
    }
    
    ixmlDocument_free(pBrowseResult);
    ixmlDocument_free(pResult);
}

ContentDirectory::SearchResult ContentDirectory::search(utils::ISubscriber<Item>& subscriber, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;
    
    Action action("Search", ContentDirectoryServiceType);
    action.addArgument("ObjectID", objectId);
    action.addArgument("SearchCriteria", criteria);
    action.addArgument("Filter", filter);
    action.addArgument("StartingIndex", numericops::toString(startIndex));
    action.addArgument("RequestedCount", numericops::toString(limit));
    action.addArgument("SortCriteria", sort);
    
    log::debug(criteria);

    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &pResult));
    
    SearchResult result;
    IXML_Document* pSearchResult = parseBrowseResult(pResult, result);
    if (!pSearchResult)
    {
        ixmlDocument_free(pResult);
        throw std::logic_error("Failed to perform search");
    }

    notifySubscriber(parseContainers(pSearchResult), subscriber);
    notifySubscriber(parseItems(pSearchResult), subscriber);
    
    ixmlDocument_free(pSearchResult);
    ixmlDocument_free(pResult);

    return result;
}

void ContentDirectory::notifySubscriber(const std::vector<Item>& items, utils::ISubscriber<Item>& subscriber)
{
    for (auto& item : items)
    {
        if (m_Abort) break;
        
        //subscriber.onItem(item, pExtraData);
        subscriber.onItem(item);
    }
}

IXML_Document* ContentDirectory::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;

    Action browseAction("Browse", ContentDirectoryServiceType);
    browseAction.addArgument("ObjectID", objectId);
    browseAction.addArgument("BrowseFlag", flag);
    browseAction.addArgument("Filter", filter);
    browseAction.addArgument("StartingIndex", numericops::toString(startIndex));
    browseAction.addArgument("RequestedCount", numericops::toString(limit));
    browseAction.addArgument("SortCriteria", sort);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, browseAction.getActionDocument(), &pResult));
    
    return pResult;
}

IXML_Document* ContentDirectory::parseBrowseResult(IXML_Document* pDoc, SearchResult& result)
{
    if (!pDoc)
    {
        return nullptr;
    }
    
    IXML_NodeList* pResultList = ixmlDocument_getElementsByTagName(pDoc, "Result");
    if (!pResultList)
    {
        return nullptr;
    }
    
    IXML_Node* pResultNode = ixmlNodeList_item(pResultList, 0);
    ixmlNodeList_free(pResultList);
    
    if (!pResultNode)
    {
        return nullptr;
    }
    
    IXML_Node* pTextNode = ixmlNode_getFirstChild(pResultNode);
    if (!pTextNode)
    {
        return nullptr;
    }
    
    std::string xml(ixmlNode_getNodeValue(pTextNode));
    IXML_Document* pBrowseDoc = ixmlParseBuffer(xml.c_str());

    result.numberReturned = stringops::toNumeric<uint32_t>(getFirstElementValue(pDoc, "NumberReturned"));
    result.totalMatches = stringops::toNumeric<uint32_t>(getFirstElementValue(pDoc, "TotalMatches"));
    
    return pBrowseDoc;
}

void ContentDirectory::parseMetaData(IXML_Document* pDoc, Item& item)
{
    if (!pDoc)
    {
        return;
    }
    
    IXML_NodeList* pItemList = ixmlDocument_getElementsByTagName(pDoc, "container");
    if (pItemList)
    {
        unsigned long numContainers = ixmlNodeList_length(pItemList);
        assert(numContainers == 1);
        
        IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pItemList, 0));
        const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
        if (pParentId)
        {
            item.setParentId(pParentId);
        }
        
        const char* pChildcount = ixmlElement_getAttribute(pItemElem, "childCount");
        if (pChildcount)
        {
            item.setChildCount(stringops::toNumeric<uint32_t>(pChildcount));
        }
        
        IXML_Node* pItemNode = ixmlNodeList_item(pItemList, 0);
        IXML_NodeList* pChildren = ixmlNode_getChildNodes(pItemNode);
        if (pChildren)
        {
            uint32_t numChildren = ixmlNodeList_length(pChildren);
            for (uint32_t i = 0; i < numChildren; ++i)
            {
                IXML_Element* pChild = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pChildren, i));
                const char* pKey = ixmlElement_getTagName(pChild);
                if (!pKey) continue;
                
                IXML_Node* pTextNode = ixmlNode_getFirstChild((IXML_Node*)pChild);
                if (!pTextNode) continue;
                
                const char* pValue = ixmlNode_getNodeValue(pTextNode);
                if (!pValue) continue;
                
                item.addMetaData(pKey, pValue);
            }
            
            ixmlNodeList_free(pChildren);
        }
        
        ixmlNodeList_free(pItemList);
    }
    
    pItemList = ixmlDocument_getElementsByTagName(pDoc, "item");
    if (pItemList)
    {
        unsigned long numContainers = ixmlNodeList_length(pItemList);
        assert(numContainers == 1);
        
        IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pItemList, 0));
        const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
        if (pParentId)
        {
            item.addMetaData("parentID", pParentId);
        }
        
        IXML_Node* pItemNode = ixmlNodeList_item(pItemList, 0);
        IXML_NodeList* pChildren = ixmlNode_getChildNodes(pItemNode);
        if (pChildren)
        {
            uint32_t numChildren = ixmlNodeList_length(pChildren);
            for (uint32_t i = 0; i < numChildren && !m_Abort; ++i)
            {
                IXML_Element* pChild = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pChildren, i));
                const char* pKey = ixmlElement_getTagName(pChild);
                if (!pKey) continue;
                
                IXML_Node* pTextNode = ixmlNode_getFirstChild((IXML_Node*)pChild);
                if (!pTextNode) continue;
                
                const char* pValue = ixmlNode_getNodeValue(pTextNode);
                if (!pValue) continue;
                
                if (std::string("res") == pKey)
                {
                    Resource resource;
                    resource.setUrl(pValue);
                    
                    IXML_NamedNodeMap* pNodeMap = ixmlNode_getAttributes(reinterpret_cast<IXML_Node*>(pChild));
                    if (pNodeMap)
                    {
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
                                    resource.setProtocolInfo(ProtocolInfo(pValue));
                                }
                                catch (std::exception& e)
                                {
                                    log::warn(e.what());
                                }
                            }
                            else
                            {
                                resource.addMetaData(pKey, pValue);
                            }
                        }
                        
                        ixmlNamedNodeMap_free(pNodeMap);
                    }
                    
                    item.addResource(resource);
                }
                else
                {
                    //log::debug(pKey, "-", pValue);
                    item.addMetaData(pKey, pValue);
                }
            }
            
            ixmlNodeList_free(pChildren);
        }
        
        ixmlNodeList_free(pItemList);
    }
}

std::vector<Item> ContentDirectory::parseContainers(IXML_Document* pDoc)
{
    std::vector<Item> containers;
    
    if (!pDoc)
    {
        return containers;
    }
    
    IXML_NodeList* pContainerList = ixmlDocument_getElementsByTagName(pDoc, "container");
    if (pContainerList)
    {
        unsigned long numContainers = ixmlNodeList_length(pContainerList);
        containers.reserve(numContainers);
        
        for (unsigned long i = 0; i < numContainers && !m_Abort; ++i)
        {
            IXML_Element* pContainerElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pContainerList, i));
            if (!pContainerElem)
            {
                log::error("Failed to get container from container list, skipping");
                continue;
            }
            
            // read attributes
            const char* pId = ixmlElement_getAttribute(pContainerElem, "id");
            if (!pId) continue;
            
            const char* pParentId = ixmlElement_getAttribute(pContainerElem, "parentID");
            if (!pParentId) continue;
            
            const char* pChildCount = ixmlElement_getAttribute(pContainerElem, "childCount");
            if (!pChildCount) continue;
            
            // read title value
            IXML_NodeList* pNodeList = ixmlElement_getElementsByTagName(pContainerElem, "dc:title");
            if (!pNodeList) continue;
            
            IXML_Node* pElement = ixmlNodeList_item(pNodeList, 0);
            ixmlNodeList_free(pNodeList);
            
            if (!pElement) continue;
            
            IXML_Node* pTextNode = ixmlNode_getFirstChild(pElement);
            if (!pTextNode) continue;
            
            const char* pTitle = ixmlNode_getNodeValue(pTextNode);
            if (!pTitle) continue;
            
            containers.push_back(Item(pId, pTitle));
            containers.back().setParentId(pParentId);
            containers.back().setChildCount(stringops::toNumeric<uint32_t>(pChildCount));
        }
        
        ixmlNodeList_free(pContainerList);
    }
    
    return containers;
}

std::vector<Item> ContentDirectory::parseItems(IXML_Document* pDoc)
{
    std::vector<Item> items;
    
    if (!pDoc)
    {
        return items;
    }
    
    IXML_NodeList* pItemList = ixmlDocument_getElementsByTagName(pDoc, "item");
    if (pItemList)
    {
        unsigned long numItems = ixmlNodeList_length(pItemList);
        for (unsigned long i = 0; i < numItems && !m_Abort; ++i)
        {
            IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pItemList, i));
            if (!pItemElem)
            {
                log::error("Failed to get item from item list, aborting");
                break;
            }
            
            // read attributes
            const char* pId = ixmlElement_getAttribute(pItemElem, "id");
            if (!pId) continue;
            
            const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
            if (!pParentId) continue;
            
            // read title value
            IXML_NodeList* pNodeList = ixmlElement_getElementsByTagName(pItemElem, "dc:title");
            if (!pNodeList) continue;
            
            IXML_Node* pElement = ixmlNodeList_item(pNodeList, 0);
            ixmlNodeList_free(pNodeList);
            
            if (!pElement) continue;
            
            IXML_Node* pTextNode = ixmlNode_getFirstChild(pElement);
            if (!pTextNode) continue;
            
            const char* pTitle = ixmlNode_getNodeValue(pTextNode);
            if (!pTitle) continue;
            
            items.push_back(Item(pId, pTitle));
            items.back().setParentId(pParentId);
        }
        
        ixmlNodeList_free(pItemList);
    }
    
    return items;
}

}
