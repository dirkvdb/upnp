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
static const uint32_t g_maxNumThreads = 20;
static const uint32_t g_requestSize =32;
    
MediaServer::MediaServer(Client& client)
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

void MediaServer::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
    m_ContentDirectory.setDevice(device);
    m_ConnectionMgr.setDevice(device); 
    
    if (m_Device->implementsService(Service::Type::AVTransport))
    {
        m_AVTransport.reset(new AVTransport(m_Client));
        m_AVTransport->setDevice(device);
    }
}

void MediaServer::abort()
{
    m_Abort = true;
    m_ContentDirectory.abort();
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

std::string MediaServer::getPeerConnectionId() const
{
    if (!m_ConnectionMgr.supportsAction(upnp::ConnectionManager::Action::PrepareForConnection))
    {
        return ConnectionManager::UnknownConnectionId;
    }
    
    std::stringstream ss;
    ss << m_Device->m_UDN << "/";
    
    if (m_Device->implementsService(Service::ConnectionManager))
    {
       ss << m_Device->m_Services[Service::ConnectionManager].m_Id;
    }
    
    return ss.str();
}

class VectorSubscriber : public utils::ISubscriber<std::shared_ptr<Item>>
{
public:
    VectorSubscriber(std::vector<std::shared_ptr<Item>>& items)
    : m_Items(items) {}

    void onItem(std::shared_ptr<Item> item, void*)
    {
        m_Items.push_back(item);
    }
    
private:
    std::vector<std::shared_ptr<Item>>&     m_Items;
};

std::vector<std::shared_ptr<Item>> MediaServer::getItemsInContainer(std::shared_ptr<Item>& container, uint32_t offset, uint32_t limit, Property sort, SortMode mode)
{
    std::vector<std::shared_ptr<Item>> items;
    VectorSubscriber sub(items);
    
    getItemsInContainer(container, sub, offset, limit, sort, mode);

    return items;
}

void MediaServer::getItemsInContainer(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::ItemsOnly, container, subscriber, offset, limit, sort, sortMode);
}

void MediaServer::getItemsInContainerAsync(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::ItemsOnly, container, std::ref(subscriber), offset, limit, sort, sortMode));
}

void MediaServer::getContainersInContainer(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::ContainersOnly, container, subscriber, offset, limit, sort, sortMode);
}

void MediaServer::getContainersInContainerAsync(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::ContainersOnly, container, std::ref(subscriber), offset, limit, sort, sortMode));
}

void MediaServer::getAllInContainer(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    performBrowseRequest(ContentDirectory::All, container, subscriber, offset, limit, sort, sortMode);
}

void MediaServer::getAllInContainerAsync(std::shared_ptr<Item>& container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::performBrowseRequestThread, this, ContentDirectory::All, container, std::ref(subscriber), offset, limit, sort, sortMode));
}

uint32_t MediaServer::search(std::shared_ptr<Item>& container, const std::string& criteria, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    m_Abort = false;
    uint32_t offset = 0;
    ContentDirectory::ActionResult res {0, 0};

    do
    {
        res = m_ContentDirectory.search(subscriber, container->getObjectId(), criteria, "*", offset, g_requestSize, "");
        offset += res.numberReturned;
    }
    while (offset < res.totalMatches || (res.totalMatches == 0 && res.numberReturned != 0));

    subscriber.finalItemReceived();

    return res.totalMatches;
}

uint32_t MediaServer::search(std::shared_ptr<Item>& container, const std::map<Property, std::string>& criteria, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
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
            throw std::logic_error("The server does not support to search on " + propertyToString(crit.first));
        }

        critString << propertyToString(crit.first) << " contains \"" << crit.second << "\"";
    }

    return search(container, critString.str(), subscriber);
}

void MediaServer::searchAsync(std::shared_ptr<Item>& container, const std::string& criteria, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::searchThread<std::string>, this, container, criteria, std::ref(subscriber)));
}

void MediaServer::searchAsync(std::shared_ptr<Item>& container, const std::map<Property, std::string>& criteria, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::searchThread<std::map<Property, std::string>>, this, container, criteria, std::ref(subscriber)));
}

void MediaServer::getMetaData(std::shared_ptr<Item>& item)
{
    m_ContentDirectory.browseMetadata(item, "*");
}

void MediaServer::getMetaDataAsync(std::shared_ptr<Item> item, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    m_ThreadPool.queueFunction(std::bind(&MediaServer::getMetaDataThread, this, item, std::ref(subscriber)));
}

void MediaServer::setTransportItem(const ConnectionInfo& info, Resource& resource)
{
    if (m_AVTransport)
    {
        m_AVTransport->setAVTransportURI(info.connectionId, resource.getUrl(), "");
    }
}

void MediaServer::performBrowseRequest(ContentDirectory::BrowseType type, std::shared_ptr<Item> container, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    m_Abort = false;

    if (sort != Property::Unknown && !canSortOnProperty(sort))
    {
        throw std::logic_error("The server does not support sort on: " + propertyToString(sort));
    }

    bool itemsLeft = true;
    uint32_t itemsReceived = 0;
    for (uint32_t curOffset = offset; itemsLeft && !m_Abort; curOffset += g_requestSize)
    {
        std::stringstream ss;
        if (sort != Property::Unknown)
        {
            ss << (sortMode == SortMode::Ascending ? "+" : "-") << propertyToString(sort);
        }
        
        uint32_t requestSize = std::min(g_requestSize, limit == 0 ? g_requestSize : limit - itemsReceived);
        ContentDirectory::ActionResult res = m_ContentDirectory.browseDirectChildren(type, subscriber, container->getObjectId(), "*", curOffset, requestSize, ss.str());
        itemsReceived += res.numberReturned;
        
        if (limit > 0)
        {
            itemsLeft = (res.numberReturned == 0) ? false : itemsReceived < limit;
        }
        else
        {
            itemsLeft = res.numberReturned == g_requestSize;
        }
    }
    
    subscriber.finalItemReceived();
}

void MediaServer::performBrowseRequestThread(ContentDirectory::BrowseType type, std::shared_ptr<Item> item, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode)
{
    try
    {
        performBrowseRequest(type, item, subscriber, offset, limit, sort, sortMode);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting items and containers:", e.what());
        subscriber.onError(e.what());
    }
}

template <typename T>
void MediaServer::searchThread(std::shared_ptr<Item> container, const T& criteria, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    try
    {
        auto critCopy = criteria;
        search(container, criteria, subscriber);
    }
    catch(std::exception& e)
    {
        log::error("Exception performing search:", e.what());
        subscriber.onError(e.what());
    }
}

void MediaServer::getMetaDataThread(std::shared_ptr<Item> item, utils::ISubscriber<std::shared_ptr<Item>>& subscriber)
{
    try
    {
        getMetaData(item);
        subscriber.onItem(item);
    }
    catch(std::exception& e)
    {
        log::error("Exception getting metadata:", e.what());
        subscriber.onError(e.what());
    }
}

ConnectionManager& MediaServer::connectionManager()
{
    return m_ConnectionMgr;
}

}
