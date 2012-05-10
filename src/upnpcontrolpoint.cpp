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

#include "upnp/upnpclient.h"
#include "upnp/upnpmediaserver.h"
#include "upnp/upnpmediarenderer.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpconnectionmanager.h"

#include "utils/log.h"

using namespace utils;

namespace upnp
{
    
ControlPoint::ControlPoint(Client& client)
: m_Client(client)
, m_Renderer(client)
, m_RendererSupportsPrepareForConnection(false)
{
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

ControlPoint::~ControlPoint()
{
}

void ControlPoint::setRendererDevice(std::shared_ptr<Device> dev)
{
    m_Renderer.setDevice(dev);
    
    m_RendererSupportsPrepareForConnection = m_Renderer.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection);
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
    
    m_Renderer.activateEvents();
}

std::shared_ptr<Device> ControlPoint::getActiveRenderer()
{
    return m_Renderer.getDevice();
}

void ControlPoint::playItem(MediaServer& server, std::shared_ptr<Item>& item)
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.stop(m_ConnInfo);
        m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
    }
    
    Resource resource;
    if (!m_Renderer.supportsPlayback(item, resource))
    {
        throw std::logic_error("The requested item is not supported by the renderer");
    }
    
    log::debug("Suggested resource: ", resource);
    
    if (m_RendererSupportsPrepareForConnection)
    {
        if (server.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection))
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

void ControlPoint::stop()
{
    if (m_ConnInfo.connectionId != ConnectionManager::UnknownConnectionId)
    {
        m_Renderer.stop(m_ConnInfo);
        m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
    }
    else
    {
        m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
        m_Renderer.stop(m_ConnInfo);
        m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
    }
}

}
