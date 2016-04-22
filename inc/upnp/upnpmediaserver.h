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
class Client2;

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

    MediaServer(Client2& client);
    ~MediaServer();

    void setDevice(const std::shared_ptr<Device>& device);
    std::shared_ptr<Device> getDevice();

    void abort();

    // Connection manager related methods
    std::string getPeerConnectionManager() const;
    void resetConnection();
    void useDefaultConnection();
    bool supportsConnectionPreparation() const;
    void prepareConnection(const Resource& resource, const std::string& peerConnectionManager, uint32_t serverConnectionId);
    uint32_t getConnectionId() const;

    // ContentDirectory related methods
    bool canSearchForProperty(Property prop) const;
    bool canSortOnProperty(Property prop) const;
    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;

    // Synchronous browse calls
    void getItemsInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getContainersInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getAllInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    std::vector<Item> getItemsInContainer(const std::string& id, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    std::vector<Item> getAllInContainer(const std::string& id, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    Item getMetaData(const std::string& id);
    void search(const std::string& id, const std::string& criteria, const ItemCb& onItem);
    void search(const std::string& id, const std::map<Property, std::string>& criteria, const ItemCb& onItem);
    std::vector<Item> search(const std::string& id, const std::string& criteria);
    std::vector<Item> search(const std::string& id, const std::map<Property, std::string>& criteria);

    // Asynchronous browse calls
    void getItemsInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getContainersInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getAllInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode mode = SortMode::Ascending);
    void getMetaDataAsync(const std::string& id, const ItemCb& onItem);
    void searchAsync(const std::string& id, const ItemCb& onItem, const std::string& criteria);
    void searchAsync(const std::string& id, const ItemCb& onItem, const std::map<Property, std::string>& criteria);

    // callbacks for the asynchronous methods
    void setItemCallback(const ItemCb& itemCb);
    void setCompletedCallback(const CompletedCb& completedCb);
    void setErrorCallback(const ErrorCb& errorCb);

    // AVTransport related methods
    void setTransportItem(Resource& resource);

    ConnectionManager::Client& connectionManager();

private:
    void performBrowseRequest(ContentDirectory::Client::BrowseType type, const std::string& id, const ItemCb& onItem, uint32_t offset = 0, uint32_t limit = 0, Property sort = Property::Unknown, SortMode = SortMode::Ascending);
    void handleSearchResult(const std::string& id, const std::string& criteria, int32_t status, uint32_t offset, const ContentDirectory::ActionResult& res, const ItemCb& cb);
    void handleBrowseResult(ContentDirectory::Client::BrowseType type,
                            const std::string& id,
                            uint32_t offset,
                            uint32_t limit,
                            const std::string& sort,
                            int32_t status,
                            const ContentDirectory::ActionResult& res,
                            const ItemCb& onItem,
                            uint32_t itemsReceived);

    std::shared_ptr<Device>                 m_device;

    Client2&                                m_client;
    ContentDirectory::Client                m_contentDirectory;
    ConnectionManager::Client               m_connectionMgr;
    std::unique_ptr<AVTransport::Client>    m_avTransport;

    ConnectionManager::ConnectionInfo       m_connInfo;

    bool                                    m_abort;

    CompletedCb                             m_completedCb;
    ErrorCb                                 m_errorCb;
};

}

#endif
