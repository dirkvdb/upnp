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

#include "upnp/upnp.mediaserver.h"

#include "upnp/upnp.item.h"
#include "upnp/upnp.device.h"

#include "utils/log.h"

#include <cmath>
#include <sstream>
#include <algorithm>

using namespace utils;

namespace upnp
{

const std::string MediaServer::rootId = "0";
static const uint32_t s_defaultRequestSize = 32;

MediaServer::MediaServer(IClient2& client)
: m_client(client)
, m_contentDirectory(client)
, m_connectionMgr(client)
, m_abort(false)
, m_requestSize(s_defaultRequestSize)
{
}

MediaServer::~MediaServer()
{
    m_abort = true;
}

void MediaServer::setDevice(const std::shared_ptr<Device>& device, std::function<void(Status)> cb)
{
    m_device = device;
    m_contentDirectory.setDevice(m_device, [this, cb] (Status status) {
        if (!status)
        {
            cb(status);
            return;
        }

        m_connectionMgr.setDevice(m_device, [this, cb] (Status status) {
            if (!status)
            {
                cb(status);
                return;
            }

            if (m_device->implementsService(ServiceType::AVTransport))
            {
                m_avTransport = std::make_unique<AVTransport::Client>(m_client);
                m_avTransport->setDevice(m_device, [this, cb] (Status status) {
                    if (!status)
                    {
                        cb(status);
                        return;
                    }

                    queryCapabilities(cb);
                });
            }
            else
            {
                queryCapabilities(cb);
            }
        });
    });
}

void MediaServer::queryCapabilities(std::function<void(Status)> cb)
{
    m_contentDirectory.querySearchCapabilities([this, cb] (Status status, const auto& caps) {
        if (!status)
        {
            log::error("Failed to obtain search capabilities");
            cb(status);
            return;
        }

        m_searchCaps = caps;

        m_contentDirectory.querySortCapabilities([this, cb] (Status status, const auto& caps) {
            if (!status)
            {
                log::error("Failed to obtain sort capabilities");
                cb(status);
                return;
            }

            m_sortCaps = caps;
            cb(status);
        });
    });
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
    ss << m_device->udn << "/";

    if (m_device->implementsService(ServiceType::ConnectionManager))
    {
        ss << m_device->services[ServiceType::ConnectionManager].id;
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
    m_connectionMgr.prepareForConnection(res.getProtocolInfo(),
                                         peerConnectionManager,
                                         remoteConnectionId,
                                         ConnectionManager::Direction::Output,
                                         [this] (Status status, ConnectionManager::ConnectionInfo info) {
        if (status)
        {
            m_connInfo = info;
        }
    });
}

uint32_t MediaServer::getConnectionId() const
{
    return m_connInfo.connectionId;
}

bool MediaServer::canSearchForProperty(Property prop) const
{
    return m_searchCaps.end() != std::find_if(m_searchCaps.begin(), m_searchCaps.end(), [&] (auto& supportedProp) {
        return (supportedProp == prop) || (supportedProp == Property::All);
    });
}

bool MediaServer::canSortOnProperty(Property prop) const
{
    return m_sortCaps.end() != std::find_if(m_sortCaps.begin(), m_sortCaps.end(), [&] (auto& supportedProp) {
        return (supportedProp == prop) || (supportedProp == Property::All);
    });
}

const std::vector<Property>& MediaServer::getSearchCapabilities() const
{
    return m_searchCaps;
}

const std::vector<Property>& MediaServer::getSortCapabilities() const
{
    return m_sortCaps;
}

void MediaServer::getItemsInContainer(const std::string& id, const ItemsCb& onItems)
{
    getItemsInContainer(id, 0, 0, Property::Unknown, SortMode::Ascending, onItems);
}

void MediaServer::getItemsInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode, const ItemsCb& onItems)
{
    performBrowseRequest(ContentDirectory::Client::ItemsOnly, id, offset, limit, sort, sortMode, onItems);
}

void MediaServer::getContainersInContainer(const std::string& id, const ItemsCb& onItems)
{
    getContainersInContainer(id, 0, 0, Property::Unknown, SortMode::Ascending, onItems);
}

void MediaServer::getContainersInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode, const ItemsCb& onItems)
{
    performBrowseRequest(ContentDirectory::Client::ContainersOnly, id, offset, limit, sort, sortMode, onItems);
}

void MediaServer::getAllInContainer(const std::string& id, const ItemsCb& onItems)
{
    getAllInContainer(id, 0, 0, Property::Unknown, SortMode::Ascending, onItems);
}

void MediaServer::getAllInContainer(const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode, const ItemsCb& onItems)
{
    performBrowseRequest(ContentDirectory::Client::All, id, offset, limit, sort, sortMode, onItems);
}

void MediaServer::handleSearchResult(const std::string& id, const std::string& criteria, uint32_t offset, uint32_t requestSize, Status status, const ContentDirectory::ActionResult& result, const ItemsCb& onItems)
{
    if (!status)
    {
        onItems(status, {});
        return;
    }

    onItems(status, result.result);
    offset += result.numberReturned;

    if (offset < result.totalMatches || (result.totalMatches == 0 && result.numberReturned != 0))
    {
        m_contentDirectory.search(id, criteria, "*", offset, requestSize, "", [=] (Status stat, const ContentDirectory::ActionResult& res) {
            handleSearchResult(id, criteria, offset, requestSize, stat, res, onItems);
        });
    }
    else if (result.totalMatches > 0)
    {
        // signal completion
        onItems(Status(), {});
    }
}

void MediaServer::search(const std::string& id, const std::string& criteria, const ItemsCb& onItems)
{
    m_abort = false;
    uint32_t offset = 0;
    auto requestSize = m_requestSize;

    m_contentDirectory.search(id, criteria, "*", offset, requestSize, "", [=] (Status status, const ContentDirectory::ActionResult& res) {
        handleSearchResult(id, criteria, offset, requestSize, status, res, onItems);
    });
}

void MediaServer::search(const std::string& id, const std::map<Property, std::string>& criteria, const ItemsCb& onItems)
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

    search(id, critString.str(), onItems);
}

void MediaServer::setTransportItem(Resource& resource)
{
    if (m_avTransport)
    {
        m_avTransport->setAVTransportURI(m_connInfo.connectionId, resource.getUrl(), "", nullptr);
    }
}

void MediaServer::setRequestSize(uint32_t size)
{
    m_requestSize = size;
}

void MediaServer::handleBrowseResult(ContentDirectory::Client::BrowseType type,
                                     const std::string& id,
                                     uint32_t offset,
                                     uint32_t limit,
                                     const std::string& sort,
                                     uint32_t requestSize,
                                     Status status,
                                     const ContentDirectory::ActionResult& result,
                                     const ItemsCb& onItems,
                                     uint32_t itemsReceived)
{
    if (!status)
    {
        onItems(status, {});
        return;
    }

    itemsReceived += result.numberReturned;
    onItems(Status(), result.result);

    bool itemsLeft = false;
    if (limit > 0)
    {
        itemsLeft = (result.numberReturned == 0) ? false : itemsReceived < limit;
    }
    else
    {
        itemsLeft = result.numberReturned == requestSize;
    }

    if (itemsLeft)
    {
        offset += m_requestSize;
        requestSize = std::min(requestSize, limit == 0 ? requestSize : limit - itemsReceived);
        log::info("Browse: {} {}", offset, requestSize);
        m_contentDirectory.browseDirectChildren(type, id, "*", offset, requestSize, sort, [=] (Status stat, const ContentDirectory::ActionResult& res) {
            handleBrowseResult(type, id, offset, limit, sort, requestSize, stat, res, onItems, itemsReceived);
        });
    }
    else if (itemsReceived > 0)
    {
        // signal completion
        onItems(Status(), {});
    }
}

void MediaServer::performBrowseRequest(ContentDirectory::Client::BrowseType type, const std::string& id, uint32_t offset, uint32_t limit, Property sort, SortMode sortMode, const ItemsCb& onItem)
{
    m_abort = false;

    if (sort != Property::Unknown && !canSortOnProperty(sort))
    {
        throw Exception("The server does not support sort on: {}", toString(sort));
    }

    std::stringstream ss;
    if (sort != Property::Unknown)
    {
        ss << (sortMode == SortMode::Ascending ? "+" : "-") << toString(sort);
    }

    auto sortStr = ss.str();
    uint32_t requestSize = std::min(m_requestSize, limit == 0 ? m_requestSize : limit);
    m_contentDirectory.browseDirectChildren(type, id, "*", offset, requestSize, sortStr, [=] (Status status, const ContentDirectory::ActionResult& res) {
        handleBrowseResult(type, id, offset, limit, sortStr, requestSize, status, res, onItem, 0);
    });
}

ConnectionManager::Client& MediaServer::connectionManager()
{
    return m_connectionMgr;
}

}
