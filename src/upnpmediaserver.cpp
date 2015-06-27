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
: m_client(client)
, m_contentDirectory(client)
, m_connectionMgr(client)
, m_threadPool(g_maxNumThreads)
, m_abort(false)
{
    m_threadPool.start();
}

MediaServer::~MediaServer()
{
    m_abort = true;
    m_threadPool.stop();
}

void MediaServer::setDevice(const std::shared_ptr<Device>& device)
{
    try
    {
        m_contentDirectory.setDevice(device);
        m_connectionMgr.setDevice(device);
        m_device = device;

        if (m_device->implementsService(ServiceType::AVTransport))
        {
            m_avTransport = std::make_unique<AVTransport::Client>(m_client);
            m_avTransport->setDevice(device);
        }
    }
    catch (std::exception& e)
    {
        throw Exception("Failed to set server device: {}", e.what());
    }
}

std::shared_ptr<Device> MediaServer::getDevice()
{
    return m_device;
}

void MediaServer::abort()
{
    m_abort = true;
    m_contentDirectory.abort();
}

std::string MediaServer::getPeerConnectionManager() const
{
    std::stringstream ss;
    ss << m_device->m_UDN << "/";

    if (m_device->implementsService(ServiceType::ConnectionManager))
    {
       ss << m_device->m_Services[ServiceType::ConnectionManager].m_Id;
    }

    return ss.str();
}

void MediaServer::resetConnection()
{
    m_connInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void MediaServer::useDefaultConnection()
{
    m_connInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

bool MediaServer::supportsConnectionPreparation() const
{
    return m_connectionMgr.supportsAction(ConnectionManager::Action::PrepareForConnection);
}

void MediaServer::prepareConnection(const Resource& res, const std::string& peerConnectionManager, uint32_t remoteConnectionId)
{
    m_connInfo = m_connectionMgr.prepareForConnection(res.getProtocolInfo(), peerConnectionManager,
                                                      remoteConnectionId, ConnectionManager::Direction::Output);
}

uint32_t MediaServer::getConnectionId() const
{
    return m_connInfo.connectionId;
}

bool MediaServer::canSearchForProperty(Property prop) const
{
    auto& props = m_contentDirectory.getSearchCapabilities();
    if (std::find(props.begin(), props.end(), prop) == props.end())
    {
        return std::find(props.begin(), props.end(), Property::All) != props.end();
    }

    return true;
}

bool MediaServer::canSortOnProperty(Property prop) const
{
    auto& props = m_contentDirectory.getSortCapabilities();

    if (std::find(props.begin(), props.end(), prop) == props.end())
    {
        return std::find(props.begin(), props.end(), Property::All) != props.end();
    }

    return true;
}

const std::vector<Property>& MediaServer::getSearchCapabilities() const
{
    return m_contentDirectory.getSearchCapabilities();
}

const std::vector<Property>& MediaServer::getSortCapabilities() const
{
    return m_contentDirectory.getSortCapabilities();
}

std::vector<Item> MediaServer::getItemsInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode mode)
{
    std::vector<Item> items;

    getItemsInContainer(id, [&items] (const Item& item) {
        items.push_back(item);
    }, offset, limit, sort, mode);

    return items;
}

std::vector<Item> MediaServer::getAllInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode mode)
{
    std::vector<Item> items;

    getAllInContainer(id, [&items] (const Item& item) {
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
    m_threadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::ItemsOnly, id, onItem, offset, limit, sort, sortMode));
}

void MediaServer::getContainersInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::Client::ContainersOnly, id, onItem, offset, limit, sort, sortMode);
}

void MediaServer::getContainersInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_threadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::ContainersOnly, id, onItem, offset, limit, sort, sortMode));
}

void MediaServer::getAllInContainer(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::Client::All, id, onItem, offset, limit, sort, sortMode);
}

void MediaServer::getAllInContainerAsync(const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_threadPool.addJob(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::Client::All, id, onItem, offset, limit, sort, sortMode));
}

std::vector<Item> MediaServer::search(const std::string& id, const std::string& criteria)
{
    std::vector<Item> items;

    search(id, criteria, [&items] (const Item& item) {
        items.push_back(item);
    });

    return items;
}

std::vector<Item> MediaServer::search(const std::string& id, const std::map<Property, std::string>& criteria)
{
    std::vector<Item> items;

    search(id, criteria, [&items] (const Item& item) {
        items.push_back(item);
    });

    return items;
}


uint32_t MediaServer::search(const std::string& id, const std::string& criteria, const ItemCb& onItem)
{
    m_abort = false;
    uint32_t offset = 0;
    ContentDirectory::ActionResult res;

    do
    {
        auto searchRes = m_contentDirectory.search(id, criteria, "*", offset, g_requestSize, "");
        for (auto& item : searchRes.result)
        {
            onItem(item);
        }

        offset += res.numberReturned;
    }
    while (offset < res.totalMatches || (res.totalMatches == 0 && res.numberReturned != 0));

    if (m_completedCb)
    {
        m_completedCb();
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
    m_threadPool.addJob(std::bind(&MediaServer::searchThread<std::string>, this, id, onItem, criteria));
}

void MediaServer::searchAsync(const std::string& id, const ItemCb& onItem, const std::map<Property, std::string>& criteria)
{
    m_threadPool.addJob(std::bind(&MediaServer::searchThread<std::map<Property, std::string>>, this, id, onItem, criteria));
}

Item MediaServer::getMetaData(const std::string& objectId)
{
    return m_contentDirectory.browseMetadata(objectId, "*");
}

void MediaServer::getMetaDataAsync(const std::string& objectId, const ItemCb& onItem)
{
    m_threadPool.addJob(std::bind(&MediaServer::getMetaDataThread, this, objectId, onItem));
}

void MediaServer::setCompletedCallback(const CompletedCb& completedCb)
{
    m_completedCb = completedCb;
}

void MediaServer::setErrorCallback(const ErrorCb& errorCb)
{
    m_errorCb = errorCb;
}

void MediaServer::setTransportItem(Resource& resource)
{
    if (m_avTransport)
    {
        m_avTransport->setAVTransportURI(m_connInfo.connectionId, resource.getUrl(), "");
    }
}

void MediaServer::performBrowseRequest(ContentDirectory::Client::BrowseType type, const std::string& id, const ItemCb& onItem, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_abort = false;

    if (sort != Property::Unknown && !canSortOnProperty(sort))
    {
        throw Exception("The server does not support sort on: {}", toString(sort));
    }

    bool itemsLeft = true;
    uint32_t itemsReceived = 0;
    for (uint32_t curOffset = offset; itemsLeft && !m_abort; curOffset += g_requestSize)
    {
        std::stringstream ss;
        if (sort != Property::Unknown)
        {
            ss << (sortMode == SortMode::Ascending ? "+" : "-") << toString(sort);
        }

        uint32_t requestSize = std::min(g_requestSize, limit == 0 ? g_requestSize : limit - itemsReceived);
        auto res = m_contentDirectory.browseDirectChildren(type, id, "*", curOffset, requestSize, ss.str());
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

    if (m_completedCb)
    {
        m_completedCb();
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
        if (m_errorCb)
        {
            m_errorCb(e.what());
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
        if (m_errorCb)
        {
            m_errorCb(e.what());
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
        if (m_errorCb)
        {
            m_errorCb(e.what());
        }
    }
}

ConnectionManager::Client& MediaServer::connectionManager()
{
    return m_connectionMgr;
}

}
