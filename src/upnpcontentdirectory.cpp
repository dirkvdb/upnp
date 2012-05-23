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
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    auto caps = stringops::tokenize(getFirstElementValue(result, "SearchCaps"), ",");
    
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
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
    auto caps = stringops::tokenize(getFirstElementValue(result, "SortCaps"), ",");
    
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
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
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
    
    //log::debug(ixmlDocumenttoString(browseResult));
    
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
    
    //log::debug(ixmlDocumenttoString(browseResult));
    
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
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, action.getActionDocument(), &result));
    
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
        
        //subscriber.onItem(item, pExtraData);
        log::debug("Item:", item->getTitle());
        subscriber.onItem(item);
    }
}

IXML_Document* ContentDirectory::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Abort = false;

    //log::debug("Browse:", objectId, flag, filter, startIndex, limit, sort);
    //log::debug(m_Device->m_Services[Service::ContentDirectory].m_ControlURL.c_str());
    
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
        
        IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, 0));
        const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
        if (pParentId)
        {
            item->setParentId(pParentId);
        }
        
        const char* pChildcount = ixmlElement_getAttribute(pItemElem, "childCount");
        if (pChildcount)
        {
            item->setChildCount(stringops::toNumeric<uint32_t>(pChildcount));
        }
        
        IXML_Node* pItemNode = ixmlNodeList_item(itemList, 0);
        IXmlNodeList children = ixmlNode_getChildNodes(pItemNode);
        if (children)
        {
            uint32_t numChildren = ixmlNodeList_length(children);
            for (uint32_t i = 0; i < numChildren; ++i)
            {
                IXML_Element* pChild = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(children, i));
                const char* pKey = ixmlElement_getTagName(pChild);
                if (!pKey) continue;
                
                IXML_Node* pTextNode = ixmlNode_getFirstChild((IXML_Node*)pChild);
                if (!pTextNode) continue;
                
                const char* pValue = ixmlNode_getNodeValue(pTextNode);
                if (!pValue) continue;
                
                item->addMetaData(pKey, pValue);
            }
        }
        
        //log::debug("-- Item with metadata --\n", *item);
    }
    
    itemList = ixmlDocument_getElementsByTagName(doc, "item");
    if (itemList)
    {
        unsigned long numContainers = ixmlNodeList_length(itemList);
        assert(numContainers == 1);
        
        IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, 0));
        const char* pParentId = ixmlElement_getAttribute(pItemElem, "parentID");
        if (pParentId)
        {
            item->addMetaData("parentID", pParentId);
        }
        
        IXML_Node* pItemNode = ixmlNodeList_item(itemList, 0);
        IXmlNodeList children = ixmlNode_getChildNodes(pItemNode);
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
                    
                    item->addResource(resource);
                }
                else
                {
                    //log::debug(pKey, "-", pValue);
                    item->addMetaData(pKey, pValue);
                }
            }
        }
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
            IXML_Element* pContainerElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(containerList, i));
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
            
            try
            {
                containers.push_back(std::make_shared<Item>(pId, getFirstElementValue(pContainerElem, "dc:title")));
                containers.back()->setParentId(pParentId);
                
                if (pChildCount)
                {
                    containers.back()->setChildCount(stringops::toNumeric<uint32_t>(pChildCount));
                }
                
                containers.back()->addMetaData("upnp:class", getFirstElementValue(pContainerElem, "upnp:class"));
                try { containers.back()->addMetaData("upnp:albumArtURI", getFirstElementValue(pContainerElem, "upnp:albumArtURI")); } catch (std::exception&) {}
                try { containers.back()->addMetaData("upnp:artist", getFirstElementValue(pContainerElem, "upnp:artist")); } catch (std::exception&) {}
                
                //log::debug("-- Container --\n", *containers.back());
            }
            catch (std::exception& e)
            {
                log::warn("Failed to parse container");
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
            IXML_Element* pItemElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(itemList, i));
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
            
            try
            {
                items.push_back(std::make_shared<Item>(pId, getFirstElementValue(pItemElem, "dc:title")));
                items.back()->setParentId(pParentId);
                
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
                            
                            items.back()->addResource(resource);
                        }
                        else
                        {
                            //log::debug(pKey, "-", pValue);
                            items.back()->addMetaData(pKey, pValue);
                        }
                    }
                }

                
                //log::debug("-- Item --\n", *items.back());
            }
            catch (std::exception& e)
            {
                log::warn("Failed to parse item");
            }
        }
    }
    
    return items;
}

}
