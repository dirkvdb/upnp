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

#include "upnp/upnpbrowser.h"

#include "upnp/upnputils.h"
#include "upnp/upnpitem.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpclient.h"

#include "utils/log.h"
#include "utils/types.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cassert>

using namespace utils;

namespace upnp
{

static const int32_t defaultTimeout = 1801;
static const char* ContentDirectoryServiceType = "urn:schemas-upnp-org:service:ContentDirectory:1";
static const uint32_t maxNumThreads = 20;

const std::string Browser::rootId = "0";

Browser::Browser(const Client& client)
: m_Client(client)
, m_ThreadPool(maxNumThreads)
, m_Stop(false)
{
    ixmlRelaxParser(1);
    m_ThreadPool.start();
}

Browser::~Browser()
{
    //unsubscribe();
    m_Stop = true;
    m_ThreadPool.stop();
}

std::shared_ptr<Device> Browser::getDevice() const
{
    return m_Device;
}

void Browser::setDevice(std::shared_ptr<Device> device)
{
    log::info("Set device:", device->m_FriendlyName);
    m_Device = device;
}

void Browser::cancel()
{
    m_Stop = true;
}

void Browser::subscribe()
{
    unsubscribe();

    log::debug("Subscribe to device:", m_Device->m_FriendlyName);

    //subscribe to eventURL
    int ret = UpnpSubscribeAsync(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), defaultTimeout, Browser::browserCb, this);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device");
    }
}

void Browser::unsubscribe()
{
    if (!m_Device->m_CDSubscriptionID.empty())
    {
        int ret = UpnpUnSubscribe(m_Client, &(m_Device->m_CDSubscriptionID[0]));
        if (ret != UPNP_E_SUCCESS)
        {
            log::warn("Failed to unsubscribe from device:", m_Device->m_FriendlyName);
        }
    }
}

void Browser::getContainersAndItemsAsync(utils::ISubscriber<Item>& subscriber, const Item& container, uint32_t limit, uint32_t offset, void* pExtraData)
{
    m_ThreadPool.queueFunction(std::bind(&Browser::getContainersAndItemsThread, this, container, limit, offset, std::ref(subscriber), pExtraData));
}

void Browser::getContainersAndItems(utils::ISubscriber<Item>& subscriber, Item& container, uint32_t limit, uint32_t offset, void* pExtraData)
{
    if (container.getChildCount() == 0)
    {
        getContainerMetaData(container);
    }
    
    uint32_t requestSize = limit == 0 ? 10 : std::min(uint32_t(10), limit);
    limit = limit == 0 ? container.getChildCount() : limit;
    uint32_t numRequests = static_cast<uint32_t>(ceilf(limit / static_cast<float>(requestSize)));
    
    for (uint32_t i = 0; i < numRequests && !m_Stop; ++i)
    {
        IXML_Document* pResult = browseAction(container.getObjectId(), "BrowseDirectChildren", "*", offset, requestSize, "");
        offset += requestSize;
        
        IXML_Document* pBrowseResult = parseBrowseResult(pResult);
        if (!pBrowseResult)
        {
            ixmlDocument_free(pResult);
            throw std::logic_error("Failed to send UPnP browse action");
        }
        
        std::vector<Item> items = parseItems(pBrowseResult);
        std::vector<Item> containers = parseContainers(pBrowseResult);
        
        ixmlDocument_free(pBrowseResult);
        ixmlDocument_free(pResult);
        
        for (auto item : items)
        {
            if (m_Stop) break;
            subscriber.onItem(item, pExtraData);
        }
        
        for (auto item : containers)
        {
            if (m_Stop) break;
            subscriber.onItem(item, pExtraData);
        }
    }
    
    subscriber.finalItemReceived(pExtraData);
}

void Browser::getContainersAsync(utils::ISubscriber<Item>& subscriber, const Item& container, uint32_t limit, uint32_t offset, void* pExtraData)
{
    m_ThreadPool.queueFunction(std::bind(&Browser::getContainersThread, this, container, limit, offset, std::ref(subscriber), pExtraData));
}

void Browser::getContainers(utils::ISubscriber<Item>& subscriber, Item& container, uint32_t limit, uint32_t offset, void* pExtraData)
{
    if (container.getChildCount() == 0)
    {
        getContainerMetaData(container);
    }

    uint32_t requestSize = limit == 0 ? 10 : std::min(uint32_t(10), limit);
    limit = limit == 0 ? container.getChildCount() : limit;
    uint32_t numRequests = static_cast<uint32_t>(ceilf(limit / static_cast<float>(requestSize)));

    for (uint32_t i = 0; i < numRequests && !m_Stop; ++i)
    {
        IXML_Document* pResult = browseAction(container.getObjectId(), "BrowseDirectChildren", "@id,@parentID,@childCount,dc:title,upnp:class", offset, requestSize, "");
        offset += requestSize;

        IXML_Document* pBrowseResult = parseBrowseResult(pResult);
        if (!pBrowseResult)
        {
            ixmlDocument_free(pResult);
            throw std::logic_error("Failed to send UPnP browse action");
        }

        std::vector<Item> containers = parseContainers(pBrowseResult);
        
        ixmlDocument_free(pBrowseResult);
        ixmlDocument_free(pResult);

        for (auto item : containers)
        {
            if (m_Stop) break;
            
            subscriber.onItem(item, pExtraData);
        }
    }

    subscriber.finalItemReceived(pExtraData);
}

void Browser::getItemsAsync(utils::ISubscriber<Item>& subscriber, const Item& container, uint32_t limit, uint32_t offset, const std::string& sort, void* pExtraData)
{
    m_ThreadPool.queueFunction(std::bind(&Browser::getItemsThread, this, container, limit, offset, sort, std::ref(subscriber), pExtraData));
}

void Browser::getItems(utils::ISubscriber<Item>& subscriber, Item& container, uint32_t limit, uint32_t offset, const std::string& sort, void* pExtraData)
{
    if (container.getChildCount() == 0)
    {
        getContainerMetaData(container);
    }

    uint32_t requestSize = 10;
    limit = limit == 0 ? container.getChildCount() : limit;
    uint32_t numRequests = static_cast<uint32_t>(ceilf(limit / static_cast<float>(requestSize)));

    for (uint32_t i = 0; i < numRequests && !m_Stop; ++i)
    {
        IXML_Document* pResult = browseAction(container.getObjectId(), "BrowseDirectChildren", "*", offset, requestSize, sort.c_str());
        offset += requestSize;

        IXML_Document* pBrowseResult = parseBrowseResult(pResult);
        if (!pBrowseResult)
        {
            ixmlDocument_free(pResult);
            throw std::logic_error("Failed to send UPnP browse action");
        }

        std::vector<Item> items = parseItems(pBrowseResult);
        
        ixmlDocument_free(pBrowseResult);
        ixmlDocument_free(pResult);

        for (auto item : items)
        {
            if (m_Stop) break;
        
            subscriber.onItem(item, pExtraData);
        }
    }
    
    subscriber.finalItemReceived(pExtraData);
}

void Browser::getMetaData(Item& item, const std::string& filter)
{
    IXML_Document* pResult = browseAction(item.getObjectId(), "BrowseMetadata", filter.empty() ? "*" : filter, 0, 0, "");
    
    IXML_Document* pBrowseResult = parseBrowseResult(pResult);
    if (!pBrowseResult)
    {
        ixmlDocument_free(pResult);
        throw std::logic_error("Failed to send UPnP browse action");
    }
    
    parseMetaData(pBrowseResult, item);
    
    ixmlDocument_free(pBrowseResult);
    ixmlDocument_free(pResult);
}

void Browser::getMetaDataAsync(utils::ISubscriber<std::shared_ptr<Item>>& subscriber, std::shared_ptr<Item> item, const std::string& filter, void* pExtraData)
{
    m_ThreadPool.queueFunction(std::bind(&Browser::getMetaDataThread, this, item, filter, std::ref(subscriber), pExtraData));
}

void Browser::getContainerMetaData(Item& container)
{
    Action browseAction("Browse", ContentDirectoryServiceType);
    browseAction.addArgument("ObjectID", container.getObjectId());
    browseAction.addArgument("BrowseFlag", "BrowseMetadata");
    browseAction.addArgument("Filter", "@childCount");
    browseAction.addArgument("StartingIndex", "0");
    browseAction.addArgument("RequestedCount", "0");
    browseAction.addArgument("SortCriteria", "");

    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::ContentDirectory].m_ControlURL.c_str(), ContentDirectoryServiceType, nullptr, browseAction.getActionDocument(), &pResult));
    
    IXML_Document* pBrowseResult = parseBrowseResult(pResult);
    if (!pBrowseResult)
    {
        throw std::logic_error("Failed to send UPnP browse action");
    }

    parseContainerMetadata(pBrowseResult, container);

    ixmlDocument_free(pBrowseResult);
    ixmlDocument_free(pResult);
}

IXML_Document* Browser::browseAction(const std::string& objectId, const std::string& flag, const std::string& filter,
                                     uint32_t startIndex, uint32_t limit, const std::string& sort)
{
    m_Stop = false;
    
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

int Browser::browserCb(Upnp_EventType eventType, void* pEvent, void* pInstance)
{
    Browser* pUPnP = reinterpret_cast<Browser*>(pInstance);

    switch (eventType)
    {
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    {
        Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
        if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Event Subscribe Callback:", pSubEvent->ErrCode);
        }
        else
        {
            if (pSubEvent->Sid)
            {
                log::info(pSubEvent->Sid);
                pUPnP->m_Device->m_CDSubscriptionID = pSubEvent->Sid;
                log::info("Successfully subscribed to", pUPnP->m_Device->m_FriendlyName, "id =", pSubEvent->Sid);
            }
            else
            {
                pUPnP->m_Device->m_CDSubscriptionID.clear();
                log::error("Subscription id for device is empty");
            }
        }

        break;
    }
    default:
        log::info("Unhandled action:", eventType);
        break;
    }

    return 0;
}

IXML_Document* Browser::parseBrowseResult(IXML_Document* pDoc)
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

    return pBrowseDoc;
}

std::vector<Item> Browser::parseContainers(IXML_Document* pDoc)
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
        
        for (unsigned long i = 0; i < numContainers; ++i)
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

std::vector<Item> Browser::parseItems(IXML_Document* pDoc)
{
    std::vector<Item> items;

    if (!pDoc)
    {
        return items;
    }

    IXML_NodeList* pItemList = ixmlDocument_getElementsByTagName(pDoc, "item");
    if (pItemList)
    {
        unsigned long numContainers = ixmlNodeList_length(pItemList);
        for (unsigned long i = 0; i < numContainers; ++i)
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


void Browser::parseContainerMetadata(IXML_Document* pDoc, Item& container)
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
        if (!pItemElem)
        {
            log::error("Failed to get root item elem, aborting");
        }
        else
        {
            // read attributes
            const char* pChildcount = ixmlElement_getAttribute(pItemElem, "childCount");
            if (pChildcount)
            {
                container.setChildCount(stringops::toNumeric<uint32_t>(pChildcount));
            }
        }

        ixmlNodeList_free(pItemList);
    }
}


void Browser::parseMetaData(IXML_Document* pDoc, Item& item)
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
                
                Property prop = propertyFromString(pKey);
                if (prop != Property::Unknown)
                {
                    item.addMetaData(prop, pValue);
                }
                else
                {
                    log::warn("Unknown property:", pKey);
                }
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
            item.setParentId(pParentId);
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

                if (std::string("res") == pKey)
                {
                    Resource resource;
                    resource.setUrl(pValue);
                    
                    IXML_NamedNodeMap* pNodeMap = ixmlNode_getAttributes(reinterpret_cast<IXML_Node*>(pChild));
                    if (pNodeMap)
                    {
                        unsigned long numAttributes = ixmlNamedNodeMap_getLength(pNodeMap);
                        for (unsigned long i = 0; i < numAttributes; ++i)
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
                
                    Property prop = propertyFromString(pKey);
                    if (prop != Property::Unknown)
                    {
                        item.addMetaData(prop, pValue);
                    }
                    else
                    {
                        log::warn("Unknown property:", pKey);
                    }
                }
            }

            ixmlNodeList_free(pChildren);
        }

        ixmlNodeList_free(pItemList);
    }
}

void Browser::getContainersAndItemsThread(const Item& container, uint32_t limit, uint32_t offset, utils::ISubscriber<Item>& subscriber, void* pData)
{
    try
    {
        Item itemCopy = container;
        getContainersAndItems(subscriber, itemCopy, limit, offset, pData);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting items and containers:", e.what());
        subscriber.onError(e.what());
    }
}

void Browser::getContainersThread(const Item& container, uint32_t limit, uint32_t offset, utils::ISubscriber<Item>& subscriber, void* pData)
{
    try
    {
        Item itemCopy = container;
        getContainers(subscriber, itemCopy, limit, offset, pData);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting containers:", e.what());
        subscriber.onError(e.what());
    }
}

void Browser::getItemsThread(const Item& container, uint32_t limit, uint32_t offset, const std::string& sort, utils::ISubscriber<Item>& subscriber, void* pData)
{
    try
    {
        Item itemCopy = container;
        getItems(subscriber, itemCopy, limit, offset, sort, pData);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting items:", e.what());
        subscriber.onError(e.what());
    }
}

void Browser::getMetaDataThread(std::shared_ptr<Item> item, const std::string& filter, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, void* pData)
{
    try
    {
        getMetaData(*item, filter);
        subscriber.onItem(item, pData);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting metadata:", e.what());
        subscriber.onError(e.what());
    }
}

}
