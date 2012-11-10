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

#include "upnp/upnpclient.h"
#include "upnp/upnpmediaserver.h"
#include "upnp/upnpmediarenderer.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpconnectionmanager.h"
#include "upnp/upnpwebserver.h"

#include "utils/log.h"

using namespace utils;

namespace upnp
{

static const uint32_t maxPlaylistSize = 100;
    
ControlPoint::ControlPoint(Client& client)
: m_Client(client)
, m_Renderer(client)
, m_pWebServer(nullptr)
, m_RendererSupportsPrepareForConnection(false)
{
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void ControlPoint::setWebserver(WebServer& webServer)
{
    m_pWebServer = &webServer;
}

void ControlPoint::setRendererDevice(const std::shared_ptr<Device>& dev)
{
    m_Renderer.setDevice(dev);
    m_RendererSupportsPrepareForConnection = m_Renderer.connectionManager().supportsAction(ConnectionManagerAction::PrepareForConnection);
    m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
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
        if (server.connectionManager().supportsAction(ConnectionManagerAction::PrepareForConnection))
        {
            m_ConnInfo = server.connectionManager().prepareForConnection(resource.getProtocolInfo(),
                                                                         ConnectionManager::UnknownConnectionId,
                                                                         m_Renderer.getPeerConnectionId(),
                                                                         Direction::Output);
        }
        
        m_ConnInfo = m_Renderer.connectionManager().prepareForConnection(resource.getProtocolInfo(),
                                                                         m_ConnInfo.connectionId,
                                                                         server.getPeerConnectionId(),
                                                                         Direction::Input);
    }
    else
    {
        m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
    }
    
    server.setTransportItem(m_ConnInfo, resource);
    m_Renderer.setTransportItem(m_ConnInfo, resource);
    m_Renderer.play(m_ConnInfo);
}

void ControlPoint::playFromItemOnwards(MediaServer& server, const std::shared_ptr<Item>& item)
{
    throwOnMissingWebserver();

    stopPlaybackIfNecessary();

    std::stringstream playlist;
    
    auto parentItem = std::make_shared<Item>(item->getParentId());
    auto items = server.getItemsInContainer(parentItem, 0, 100);
    
    auto iter = std::find_if(items.begin(), items.end(), [item](const std::shared_ptr<Item>& i) { return i->getObjectId() == item->getObjectId(); });
    std::for_each(iter, items.end(), [this, &playlist] (const std::shared_ptr<Item>& i) {
        Resource resource;
        if (m_Renderer.supportsPlayback(i, resource))
        {
            playlist << resource.getUrl() << std::endl;
        }
    });
    
    std::string filename = generatePlaylistFilename();
    m_pWebServer->clearFiles();
    m_pWebServer->addFile(filename, playlist.str());
    
    auto playlistItem = createPlaylistItem(filename);
    
    playItem(server, playlistItem);
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
    m_pWebServer->addFile(filename, playlist.str());
    
    auto playlistItem = createPlaylistItem(filename);
    playItem(server, playlistItem);
}

void ControlPoint::resume()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.play(m_ConnInfo);
    }
}

void ControlPoint::pause()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.pause(m_ConnInfo);
    }
}

void ControlPoint::stop()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.stop(m_ConnInfo);
    }
}

void ControlPoint::next()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.next(m_ConnInfo);
    }
}

void ControlPoint::previous()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.previous(m_ConnInfo);
    }
}

void ControlPoint::setVolume(uint32_t value)
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.setVolume(m_ConnInfo, value);
    }
}

uint32_t ControlPoint::getVolume()
{
    return m_Renderer.getVolume();
}

void ControlPoint::stopPlaybackIfNecessary()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        if (m_Renderer.isActionAvailable(MediaRenderer::Action::Stop))
        {
            m_Renderer.stop(m_ConnInfo);
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
    res.setUrl(m_pWebServer->getWebRootUrl() + filename);
    res.setProtocolInfo(ProtocolInfo("http-get:*:audio/m3u:*"));
    
    auto playlistItem = std::make_shared<Item>();
    playlistItem->addResource(res);
    
    return playlistItem;
}

}
