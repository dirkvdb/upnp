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

#include "upnp/upnpcontrolpoint.h"

#include <chrono>
#include <condition_variable>

#include "upnp/upnpclient.h"
#include "upnp/upnpmediaserver.h"
#include "upnp/upnpmediarenderer.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpconnectionmanagerclient.h"
#include "upnp/upnpwebserver.h"

#include "utils/log.h"

using namespace utils;

namespace upnp
{

static const uint32_t maxPlaylistSize = 100;
    
ControlPoint::ControlPoint(Client& client)
: m_Renderer(client)
, m_pWebServer(nullptr)
, m_RendererSupportsPrepareForConnection(false)
{
    m_ServerConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
    m_RendererConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void ControlPoint::setWebserver(WebServer& webServer)
{
    m_pWebServer = &webServer;
    m_pWebServer->addVirtualDirectory("playlists");
}

void ControlPoint::setRendererDevice(const std::shared_ptr<Device>& dev)
{
    m_Renderer.setDevice(dev);
    m_RendererSupportsPrepareForConnection = m_Renderer.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection);
    m_ServerConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
    m_RendererConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

MediaRenderer& ControlPoint::getActiveRenderer()
{
    return m_Renderer;
}

void ControlPoint::activate()
{
    m_Renderer.activateEvents();
}

void ControlPoint::deactivate()
{
    m_Renderer.deactivateEvents();
}

void ControlPoint::playItem(MediaServer& server, const std::shared_ptr<Item>& item)
{
    stopPlaybackIfNecessary();
    
    Resource resource;
    if (!m_Renderer.supportsPlayback(item, resource))
    {
        throw std::logic_error("The requested item is not supported by the renderer");
    }
    
    if (m_RendererSupportsPrepareForConnection)
    {
        if (server.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection))
        {
            m_ServerConnInfo = server.connectionManager().prepareForConnection(resource.getProtocolInfo(),
                                                                               m_Renderer.getPeerConnectionManager(),
                                                                               ConnectionManager::UnknownConnectionId,
                                                                               ConnectionManager::Direction::Output);
        }
        
        m_RendererConnInfo = m_Renderer.connectionManager().prepareForConnection(resource.getProtocolInfo(),
                                                                                 server.getPeerConnectionManager(),
                                                                                 m_ServerConnInfo.connectionId,
                                                                                 ConnectionManager::Direction::Input);
    }
    else
    {
        m_ServerConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
        m_RendererConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
    }
    
    server.setTransportItem(m_ServerConnInfo, resource);
    m_Renderer.setTransportItem(m_RendererConnInfo, resource);
    m_Renderer.play(m_RendererConnInfo);
}

void ControlPoint::playFromItemOnwards(MediaServer& server, const std::shared_ptr<Item>& item)
{
    throwOnMissingWebserver();

    stopPlaybackIfNecessary();

    auto parentItem = std::make_shared<Item>(item->getParentId());
    auto items = server.getItemsInContainer(parentItem, 0, 100);
    
    std::vector<std::shared_ptr<Item>> supportedItems;
    
    auto iter = std::find_if(items.begin(), items.end(), [item](const std::shared_ptr<Item>& i) { return i->getObjectId() == item->getObjectId(); });
    std::for_each(iter, items.end(), [&] (const std::shared_ptr<Item>& i) {
        Resource resource;
        if (m_Renderer.supportsPlayback(i, resource))
        {
            supportedItems.push_back(i);
        }
    });
    
    
    if (supportedItems.empty())
    {
        throw std::logic_error("No supported items");
    }
    else if (supportedItems.size() == 1)
    {
        playItem(server, supportedItems.front());
    }
    else
    {
        std::stringstream playlist;
        for (auto& i : supportedItems)
        {
            Resource res;
            if (m_Renderer.supportsPlayback(i, res))
            {
                playlist << res.getUrl() << std::endl;
            }
        }
    
        std::string filename = generatePlaylistFilename();
        m_pWebServer->clearFiles();
        m_pWebServer->addFile("playlists", filename, "audio/m3u", playlist.str());
        
        auto playlistItem = createPlaylistItem(filename);
        
        playItem(server, playlistItem);
    }
}

void ControlPoint::playContainer(MediaServer& server, const std::shared_ptr<Item>& container)
{
    throwOnMissingWebserver();
    
    stopPlaybackIfNecessary();

    auto items = server.getItemsInContainer(container);
    std::stringstream playlist;
    for (auto& item : items)
    {
        Resource resource;
        if (m_Renderer.supportsPlayback(item, resource))
        {
            playlist << resource.getUrl() << std::endl;
        }
    }
    
    std::string filename = generatePlaylistFilename();
    m_pWebServer->clearFiles();
    m_pWebServer->addFile("playlists", filename, "audio/m3u", playlist.str());
    
    auto playlistItem = createPlaylistItem(filename);
    playItem(server, playlistItem);
}

void ControlPoint::resume()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.play(m_RendererConnInfo);
    }
}

void ControlPoint::pause()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.pause(m_RendererConnInfo);
    }
}

void ControlPoint::stop()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.stop(m_RendererConnInfo);
    }
}

void ControlPoint::next()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.next(m_RendererConnInfo);
    }
}

void ControlPoint::previous()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.previous(m_RendererConnInfo);
    }
}

void ControlPoint::setVolume(uint32_t value)
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.setVolume(m_RendererConnInfo, value);
    }
}

uint32_t ControlPoint::getVolume()
{
    return m_Renderer.getVolume();
}

void ControlPoint::stopPlaybackIfNecessary()
{
    if (m_RendererConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        if (m_Renderer.isActionAvailable(MediaRenderer::Action::Stop))
        {
            m_Renderer.stop(m_RendererConnInfo);
        }
    }
}

void ControlPoint::throwOnMissingWebserver()
{
    if (!m_pWebServer)
    {
        throw std::logic_error("A web server must be available to serve playlists");
    }
}

std::string ControlPoint::generatePlaylistFilename()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    
    std::stringstream playlistFilename;
    playlistFilename << "playlist-" << now.time_since_epoch().count() << ".m3u";
    
    return playlistFilename.str();
}

std::shared_ptr<Item> ControlPoint::createPlaylistItem(const std::string& filename)
{
    Resource res;
    res.setUrl(m_pWebServer->getWebRootUrl() + "playlists/" + filename);
    res.setProtocolInfo(ProtocolInfo("http-get:*:audio/m3u:*"));
    
    auto playlistItem = std::make_shared<Item>();
    playlistItem->addResource(res);
    
    return playlistItem;
}

}
