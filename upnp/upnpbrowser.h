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

#ifndef UPNP_BROWSER_H
#define UPNP_BROWSER_H

#include <string>
#include <list>
#include <map>
#include <deque>
#include <future>
#include <mutex>
#include <memory>
#include <upnp/upnp.h>

#include "upnpitem.h"
#include "upnpcontrolpoint.h"

namespace UPnP
{

class UPnPControlPoint;


class Browser
{
public:
    Browser(const ControlPoint& ctrlPnt);
    ~Browser();

    Device getDevice() const;
    void setDevice(const Device& device);
    
    // fetch the child containers and items of container with id 'containerId', if the containers' metadata is not known yet it will be also be fetched
    void getContainersAndItems(utils::ISubscriber<Item>& subscriber, Item& container, uint32_t limit = 0, uint32_t offset = 0, void* pExtraData = nullptr);
    void getContainersAndItemsAsync(utils::ISubscriber<Item>& subscriber, const Item& container, uint32_t limit = 0, uint32_t offset = 0, void* pExtraData = nullptr);

    // fetch the child containers of container with id 'containerId', if the containers' metadata is not known yet it will be also be fetched
    void getContainers(utils::ISubscriber<Item>& subscriber, Item& container, uint32_t limit = 0, uint32_t offset = 0, void* pExtraData = nullptr);
    void getContainersAsync(utils::ISubscriber<Item>& subscriber, const Item& container, uint32_t limit = 0, uint32_t offset = 0, void* pExtraData = nullptr);

    // fetch the child items of container with id 'containerId', if the containers' metadata is not known yet it will be also be fetched
    void getItems(utils::ISubscriber<Item>& pSubscriber, Item& container, uint32_t limit = 0, uint32_t offset = 0, const std::string& sort = "", void* pExtraData = nullptr);
    void getItemsAsync(utils::ISubscriber<Item>& pSubscriber, const Item& container, uint32_t limit = 0, uint32_t offset = 0, const std::string& sort = "", void* pExtraData = nullptr);

    // fetch the metadata of the given item
    void getMetaData(Item& item, const std::string& filter);
    void getMetaDataAsync(utils::ISubscriber<std::shared_ptr<Item>>& subscriber, std::shared_ptr<Item> item, const std::string& filter, void* pExtraData = nullptr);

    static const std::string rootId;

private:
    void abortThreads();
    void subscribe();
    void unsubscribe();
    void getContainerMetaData(Item& container);
    void processThreadQueue();

    void parseContainerMetadata(IXML_Document* pDoc, Item& container);
    void parseMetaData(IXML_Document* pDoc, Item& item);
    std::vector<Item> parseContainers(IXML_Document* pDoc);
    std::vector<Item> parseItems(IXML_Document* pDoc);
    
    IXML_Document* browseAction(const std::string& objectId, const std::string& flag, const std::string& filter,
                                uint32_t startIndex, uint32_t limit, const std::string& sort);


    static IXML_Document* parseBrowseResult(IXML_Document* pDoc);
    static void addActionToDocument(IXML_Document** pDoc, const std::string& type, const std::string& action, const std::string& value);
    static void handleUPnPError(int errorCode);

    static int browserCb(Upnp_EventType eventType, void* pEvent, void* pInstance);
    
    void getContainersAndItemsThread(const Item& container, uint32_t limit, uint32_t offset, utils::ISubscriber<Item>& subscriber, void* pData);
    void getContainersThread(const Item& container, uint32_t limit, uint32_t offset, utils::ISubscriber<Item>& subscriber, void* pData);
    void getItemsThread(const Item& container, uint32_t limit, uint32_t offset, const std::string& sort, utils::ISubscriber<Item>& subscriber, void* pData);
    void getMetaDataThread(std::shared_ptr<Item> item, const std::string& filter, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, void* pData);

    const ControlPoint&                 m_CtrlPnt;
    Device                              m_Device;
    std::string                         m_SubscriptionId;
    std::mutex                          m_ThreadListMutex;
    std::list<std::future<void>>        m_RunningThreads;
    std::deque<std::function<void ()>>  m_QueuedThreads;
    bool                                m_Stop;
};

}

#endif
