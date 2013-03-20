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

#ifndef UPNP_MEDIA_SERVER_H
#define UPNP_MEDIA_SERVER_H

#include <memory>
#include <vector>

#include "upnp/upnpconnectionmanagerclient.h"
#include "upnp/upnpcontentdirectoryclient.h"
#include "upnp/upnpavtransportclient.h"

#include "utils/threadpool.h"

namespace upnp
{
    
class Item;
class Device;
class IClient;

class MediaServer
{
public:
    static const std::string rootId;
    
    typedef std::function<void()> CompletedCb;
    typedef std::function<void(const std::string&)> ErrorCb;

    enum class SortMode
    {
        Ascending,
        Descending
    };

    MediaServer(IClient& client);
    ~MediaServer();
    
    void setDevice(const std::shared_ptr<Device>& device);
    std::shared_ptr<Device> getDevice();
    
    void abort();
    
    // ContentDirectory related methods
    bool canSearchForProperty(Property prop) const;
    bool canSortOnProperty(Property prop) const;
    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;
    std::string getPeerConnectionId() const;
    
    std::vector<ItemPtr> getItemsInContainer(const ItemPtr& container, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    
    // Synchronous browse calls
    void getItemsInContainer            (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getContainersInContainer       (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getAllInContainer              (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getMetaData                    (const ItemPtr& item);
    uint32_t search                     (const ItemPtr& container, const std::string& criteria, const ItemCb& onItem);
    uint32_t search                     (const ItemPtr& container, const std::map<Property, std::string>& criteria, const ItemCb& onItem);
    std::vector<ItemPtr> search         (const ItemPtr& container, const std::string& criteria);
    std::vector<ItemPtr> search         (const ItemPtr& container, const std::map<Property, std::string>& criteria);
    
    // Asynchronous browse calls
    void getItemsInContainerAsync       (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getContainersInContainerAsync  (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getAllInContainerAsync         (const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getMetaDataAsync               (const ItemPtr& item, const ItemCb& onItem);
    void searchAsync                    (const ItemPtr& container, const ItemCb& onItem, const std::string& criteria);
    void searchAsync                    (const ItemPtr& container, const ItemCb& onItem, const std::map<Property, std::string>& criteria);
    
    // callbacks for the asynchronous methods
    void setItemCallback(const ItemCb& itemCb);
    void setCompletedCallback(const CompletedCb& completedCb);
    void setErrorCallback(const ErrorCb& errorCb);
        
    
    
    // AVTransport related methods
    void setTransportItem(const ConnectionInfo& info, Resource& resource);
    
    ConnectionManager::Client& connectionManager();
    
private:
    void performBrowseRequest(ContentDirectory::Client::BrowseType type, const ItemPtr& container, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode = SortMode::Ascending);
    void performBrowseRequestThread(ContentDirectory::Client::BrowseType type, const ItemPtr& item, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode = SortMode::Ascending);
    template <typename T>
    void searchThread(const ItemPtr& container, const ItemCb& onItem, const T& criteria);
    void getMetaDataThread(const ItemPtr& item, const ItemCb& onItem);

    std::shared_ptr<Device>                 m_Device;
    std::vector<ProtocolInfo>               m_ProtocolInfo;
    
    IClient&                                m_Client;
    ContentDirectory::Client                m_ContentDirectory;
    ConnectionManager::Client               m_ConnectionMgr;
    std::unique_ptr<AVTransport::Client>    m_AVTransport;
    
    utils::ThreadPool                       m_ThreadPool;
    bool                                    m_Abort;
    
    ItemCb                                  m_ItemCb;
    CompletedCb                             m_CompletedCb;
    ErrorCb                                 m_ErrorCb;
};
    
}

#endif
