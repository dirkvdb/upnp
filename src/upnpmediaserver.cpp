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

#include "upnp/upnpmediaserver.h"

#include "upnp/upnpitem.h"
#include "upnp/upnpdevice.h"

#include "utils/log.h"

#include <cmath>
#include <sstream>
#include <algorithm>

using namespace utils;

namespace upnp
{

const std::string MediaServer::rootId = "0";
static const uint32_t g_maxNumThreads = 8;
static const uint32_t g_requestSize =32;
    
MediaServer::MediaServer(IClient& client)
: m_Client(client)
, m_ContentDirectory(client)
, m_ConnectionMgr(client)
, m_ThreadPool(g_maxNumThreads)
, m_Abort(false)
{
    m_ThreadPool.start();
}

MediaServer::~MediaServer()
{
    m_Abort = true;
    m_ThreadPool.stop();
}

void MediaServer::setDevice(const std::shared_ptr<Device>& device)
{
    try
    {
        m_ContentDirectory.setDevice(device);
        m_ConnectionMgr.setDevice(device);
        m_Device = device;
        
        if (m_Device->implementsService(ServiceType::AVTransport))
        {
            m_AVTransport = std::make_unique<AVTransport::Client>(m_Client);
            m_AVTransport->setDevice(device);
        }
    }
    catch (std::exception& e)
    {
        throw Exception("Failed to set server device: {}", e.what());
    }
}

std::shared_ptr<Device> MediaServer::getDevice()
{
    return m_Device;
}

void MediaServer::abort()
{
    m_Abort = true;
    m_ContentDirectory.abort();
}

std::string MediaServer::getPeerConnectionManager() const
{
    std::stringstream ss;
    ss << m_Device->m_UDN << "/";
    
    if (m_Device->implementsService(ServiceType::ConnectionManager))
    {
       ss << m_Device->m_Services[ServiceType::ConnectionManager].m_Id;
    }
    
    return ss.str();
}

void MediaServer::resetConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void MediaServer::useDefaultConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

bool MediaServer::supportsConnectionPreparation() const
{
    return m_ConnectionMgr.supportsAction(ConnectionManager::Action::PrepareForConnection);
}

void MediaServer::prepareConnection(const Resource& res, const std::string& peerConnectionManager, uint32_t remoteConnectionId)
{
    m_ConnInfo = m_ConnectionMgr.prepareForConnection(res.getProtocolInfo(), peerConnectionManager,
                                                      remoteConnectionId, ConnectionManager::Direction::Output);
}

uint32_t MediaServer::getConnectionId() const
{
    return m_ConnInfo.connectionId;
}

bool MediaServer::canSearchForProperty(Property prop) const
{
    auto& props = m_ContentDirectory.getSearchCapabilities();
    if (std::find(props.begin(), props.end(), prop) == props.end())
    {
        return std::find(props.begin(), props.end(), Property::All) != props.end();
    }
    
    return true;
}

bool MediaServer::canSortOnProperty(Property prop) const
{
    auto& props = m_ContentDirectory.getSortCapabilities();
    
    if (std::find(props.begin(), props.end(), prop) == props.end())
    {
        return std::find(props.begin(), props.end(), Property::All) != props.end();
    }
    
    return true;
}

const std::vector<Property>& MediaServer::getSearchCapabilities() const
{
    return m_ContentDirectory.getSearchCapabilities();
}

const std::vector<Property>& MediaServer::getSortCapabilities() const
{
    return m_ContentDirectory.getSortCapabilities();
}

std::vector<ItemPtr> MediaServer::getItemsInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode mode)
{
    std::vector<ItemPtr> items;
    
    getItemsInContainer(id, [&items] (const ItemPtr& item) {
        items.push_back(item);
    }, offset, limit, sort, mode);

    return items;
}
    
std::vector<ItemPtr> MediaServer::getAllInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode mode)
{
    std::vector<ItemPtr> items;
    
    getAllInContainer(id, [&items] (const ItemPtr& item) {
        items.push_back(item);
    }, offset, limit, sort, mode);
    
    return items;
}

void MediaServer::getItemsInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::Client::ItemsOnly, id, onItem, offset, limit, sort, sortMode);
}

void MediaServer::getItemsInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::ItemsOnly, id, onItem, offset, limit, sort, sortMode));
}

void MediaServer::getContainersInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::Client::ContainersOnly, id, onItem, offset, limit, sort, sortMode);
}

void MediaServer::getContainersInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::ContainersOnly, id, onItem, offset, limit, sort, sortMode));
}

void MediaServer::getAllInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::Client::All, id, onItem, offset, limit, sort, sortMode);
}

void MediaServer::getAllInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::All, id, onItem, offset, limit, sort, sortMode));
}

std::vector<ItemPtr> MediaServer::search(const std::string& id, const std::string& criteria)
{
    std::vector<ItemPtr> items;
    
    search(id, criteria, [&items] (const ItemPtr& item) {
        items.push_back(item);
    });
    
    return items;
}

std::vector<ItemPtr> MediaServer::search(const std::string& id, const std::map<Property, std::string>& criteria)
{
    std::vector<ItemPtr> items;
    
    search(id, criteria, [&items] (const ItemPtr& item) {
        items.push_back(item);
    });
    
    return items;
}
    

uint32_t MediaServer::search(const std::string& id, const std::string& criteria, const ItemCb& onItem)
{
    m_Abort = false;
    uint32_t offset = 0;
    ContentDirectory::ActionResult res;

    do
    {
        auto searchRes = m_ContentDirectory.search(id, criteria, "*", offset, g_requestSize, "");
        for (auto& item : searchRes.result)
        {
            onItem(item);
        }
        
        offset += res.numberReturned;
    }
    while (offset < res.totalMatches || (res.totalMatches == 0 && res.numberReturned != 0));

    if (m_CompletedCb)
    {
        m_CompletedCb();
    }

    return res.totalMatches;
}

uint32_t MediaServer::search(const std::string& id, const std::map<Property, std::string>& criteria, const ItemCb& onItem)
{
    bool first = true;
    std::stringstream critString;
    for (auto& crit : criteria)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            critString << " and ";
        }        
        
        if (!canSearchForProperty(crit.first))
        {
            throw Exception("The server does not support to search on {}", toString(crit.first));
        }

        critString << toString(crit.first) << " contains \"" << crit.second << "\"";
    }

    return search(id, critString.str(), onItem);
}

void MediaServer::searchAsync(const std::string& id, const ItemCb& onItem, const std::string& criteria)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::searchThread<std::string>, this, id, onItem, criteria));
}

void MediaServer::searchAsync(const std::string& id, const ItemCb& onItem, const std::map<Property, std::string>& criteria)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::searchThread<std::map<Property, std::string>>, this, id, onItem, criteria));
}

ItemPtr MediaServer::getMetaData(const std::string& objectId)
{
    return m_ContentDirectory.browseMetadata(objectId, "*");
}

void MediaServer::getMetaDataAsync(const std::string& objectId, const ItemCb& onItem)
{
    m_ThreadPool.addJob(std::bind(&MediaServer::getMetaDataThread, this, objectId, onItem));
}

void MediaServer::setCompletedCallback(const CompletedCb& completedCb)
{
    m_CompletedCb = completedCb;
}

void MediaServer::setErrorCallback(const ErrorCb& errorCb)
{
    m_ErrorCb = errorCb;
}

void MediaServer::setTransportItem(Resource& resource)
{
    if (m_AVTransport)
    {
        m_AVTransport->setAVTransportURI(m_ConnInfo.connectionId, resource.getUrl(), "");
    }
}

void MediaServer::performBrowseRequest(ContentDirectory::Client::BrowseType type, const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_Abort = false;

    if (sort != Property::Unknown && !canSortOnProperty(sort))
    {
        throw Exception("The server does not support sort on: {}", toString(sort));
    }

    bool itemsLeft = true;
    uint32_t itemsReceived = 0;
    for (uint32_t curOffset = offset; itemsLeft && !m_Abort; curOffset += g_requestSize)
    {
        std::stringstream ss;
        if (sort != Property::Unknown)
        {
            ss << (sortMode == SortMode::Ascending ? "+" : "-") << toString(sort);
        }
        
        uint32_t requestSize = std::min(g_requestSize, limit == 0 ? g_requestSize : limit - itemsReceived);
        auto res = m_ContentDirectory.browseDirectChildren(type, id, "*", curOffset, requestSize, ss.str());
        itemsReceived += res.numberReturned;
        for (auto& item : res.result)
        {
            onItem(item);
        }
        
        if (limit > 0)
        {
            itemsLeft = (res.numberReturned == 0) ? false : itemsReceived < limit;
        }
        else
        {
            itemsLeft = res.numberReturned == g_requestSize;
        }
    }
    
    if (m_CompletedCb)
    {
        m_CompletedCb();
    }
}

void MediaServer::performBrowseRequestThread(ContentDirectory::Client::BrowseType type, const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    try
    {
        performBrowseRequest(type, id, onItem, offset, limit, sort, sortMode);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting items and containers: {}", e.what());
        if (m_ErrorCb)
        {
            m_ErrorCb(e.what());
        }
    }
}

template <typename T>
void MediaServer::searchThread(const std::string& id, const ItemCb& onItem, const T& criteria)
{
    try
    {
        auto critCopy = criteria;
        search(id, criteria, onItem);
    }
    catch(std::exception& e)
    {
        log::error("Exception performing search: {}", e.what());
        if (m_ErrorCb)
        {
            m_ErrorCb(e.what());
        }
    }
}

void MediaServer::getMetaDataThread(const std::string& id, const ItemCb& onItem)
{
    try
    {
        onItem(getMetaData(id));
    }
    catch(std::exception& e)
    {
        log::error("Exception getting metadata: {}", e.what());
        if (m_ErrorCb)
        {
            m_ErrorCb(e.what());
        }
    }
}

ConnectionManager::Client& MediaServer::connectionManager()
{
    return m_ConnectionMgr;
}

}
