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

#include "upnp/upnpmediarenderer.h"

#include "upnp/upnpitem.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include <sstream>

using namespace utils;
using namespace std::placeholders;


namespace upnp
{

static const std::string g_NotImplemented = "NOT_IMPLEMENTED";

MediaRenderer::MediaRenderer(IClient& client)
: m_Client(client)
, m_ConnectionMgr(client)
, m_RenderingControl(client)
, m_Active(false)
, m_CurrentVolume(0)
{
}

std::shared_ptr<Device> MediaRenderer::getDevice()
{
    return m_Device;
}

void MediaRenderer::setDevice(const std::shared_ptr<Device>& device)
{
    try
    {
        if (m_Device)
        {
            deactivateEvents();
        }

        m_Device = device;
        m_ConnectionMgr.setDevice(device);
        m_RenderingControl.setDevice(device);
        
        if (m_Device->implementsService(ServiceType::AVTransport))
        {
            m_AVtransport.reset(new AVTransport::Client(m_Client));
            m_AVtransport->setDevice(device);
        }
        
        m_ProtocolInfo = m_ConnectionMgr.getProtocolInfo();
        
        activateEvents();
    }
    catch (std::exception& e)
    {
        throw std::logic_error(stringops::format("Failed to set renderer device: %s", e.what()));
    }
}

bool MediaRenderer::supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const
{
    if (!m_Device)
    {
        throw std::logic_error("No UPnP renderer selected");
    }

    for (auto& res : item->getResources())
    {
        auto iter = std::find_if(m_ProtocolInfo.begin(), m_ProtocolInfo.end(), [res] (const ProtocolInfo& info) {
            return info.isCompatibleWith(res.getProtocolInfo());
        });
        
        if (iter != m_ProtocolInfo.end())
        {
            suggestedResource = res;
            return true;
        }
    }
    
    return false;
}

std::string MediaRenderer::getPeerConnectionManager() const
{
    std::stringstream ss;
    ss << m_Device->m_UDN << "/" << m_Device->m_Services[ServiceType::ConnectionManager].m_Id;
    
    return ss.str();
}

void MediaRenderer::resetConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void MediaRenderer::useDefaultConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

bool MediaRenderer::supportsConnectionPreparation() const
{
    return m_ConnectionMgr.supportsAction(ConnectionManager::Action::PrepareForConnection);
}

void MediaRenderer::prepareConnection(const Resource& res, const std::string& peerConnectionManager, uint32_t serverConnectionId)
{
    m_ConnInfo = m_ConnectionMgr.prepareForConnection(res.getProtocolInfo(), peerConnectionManager,
                                                      serverConnectionId, ConnectionManager::Direction::Input);
}

void MediaRenderer::setTransportItem(Resource& resource)
{
    if (m_AVtransport)
    {
        m_AVtransport->setAVTransportURI(m_ConnInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::setNextTransportItem(Resource& resource)
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->setNextAVTransportURI(m_ConnInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::play()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->play(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::pause()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->pause(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::stop()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->stop(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::next()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->next(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::previous()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->previous(m_ConnInfo.connectionId);
    }
}

std::string MediaRenderer::getCurrentTrackPosition()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        auto transportInfo = m_AVtransport->getPositionInfo(m_ConnInfo.connectionId);
        return transportInfo.relTime;
    }
    
    return "";
}

std::string MediaRenderer::getCurrentTrackURI() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTrackURI);
    return iter == m_AvTransportInfo.end() ? "" : iter->second;
}

std::string MediaRenderer::getCurrentTrackDuration() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTrackDuration);
    return iter == m_AvTransportInfo.end() ? "" : iter->second;
}

Item MediaRenderer::getCurrentTrackInfo() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    Item item;

    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTrackDuration);
    if (iter != m_AvTransportInfo.end())
    {
        log::warn("Meta parsing not implemented");
    }
    
    return item;
}

bool MediaRenderer::isActionAvailable(Action action) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_AvailableActions.find(action) != m_AvailableActions.end();
}

bool MediaRenderer::supportsQueueItem() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_AVtransport ? m_AVtransport->supportsAction(AVTransport::Action::SetNextAVTransportURI) : false;
}

void MediaRenderer::setVolume(uint32_t value)
{
    throwOnUnknownConnectionId();
    m_RenderingControl.setVolume(m_ConnInfo.connectionId, value);
}

uint32_t MediaRenderer::getVolume()
{
    return m_CurrentVolume;
}

void MediaRenderer::activateEvents()
{
    if (!m_Active && m_Device)
    {
        m_RenderingControl.LastChangeEvent.connect(std::bind(&MediaRenderer::onRenderingControlLastChangeEvent, this, _1), this);
        m_RenderingControl.subscribe();

        if (m_AVtransport)
        {
            m_AVtransport->LastChangeEvent.connect(std::bind(&MediaRenderer::onAVTransportLastChangeEvent, this, _1), this);
            m_AVtransport->subscribe();
        }
        
        m_Active = true;
    }
}

void MediaRenderer::deactivateEvents()
{
    if (m_Active && m_Device)
    {
        try
        {
            m_RenderingControl.StateVariableEvent.disconnect(this);
            m_RenderingControl.unsubscribe();
        }
        catch (std::logic_error& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }
    
        try
        {
            if (m_AVtransport)
            {
                m_AVtransport->StateVariableEvent.disconnect(this);
                m_AVtransport->unsubscribe();
            }
        }
        catch (std::logic_error& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }
        
        m_Active = false;
    }
}

void MediaRenderer::calculateAvailableActions()
{
    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTransportActions);
    if (iter == m_AvTransportInfo.end())
    {
        // no action info provided, allow everything
        m_AvailableActions =  { Action::Play, Action::Stop, Action::Pause, Action::Seek,
                                Action::Next, Action::Previous, Action::Record };
        return;
    }
    
    m_AvailableActions.clear();
    
    auto actionsStrings = stringops::tokenize(iter->second, ",");
    std::for_each(actionsStrings.begin(), actionsStrings.end(), [&] (const std::string& action) {
        try { m_AvailableActions.insert(transportActionToAction(m_AVtransport->actionFromString(action))); }
        catch (std::exception& e) { log::warn(e.what()); }
    });
}

void MediaRenderer::onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = vars.find(RenderingControl::Variable::Volume);
    if (iter != vars.end())
    {
        m_CurrentVolume = utils::stringops::toNumeric<uint32_t>(iter->second);
        VolumeChanged(m_CurrentVolume);
    }
}

void MediaRenderer::onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto& pair : vars)
    {
        m_AvTransportInfo[pair.first] = pair.second;
    }

    if (vars.find(AVTransport::Variable::CurrentTransportActions) != vars.end())
    {
        calculateAvailableActions();
    }
    
    MediaInfoChanged();
}

MediaRenderer::Action MediaRenderer::transportActionToAction(AVTransport::Action action)
{
    switch (action)
    {
        case AVTransport::Action::Play:     return Action::Play;
        case AVTransport::Action::Stop:     return Action::Stop;
        case AVTransport::Action::Pause:    return Action::Pause;
        case AVTransport::Action::Seek:     return Action::Seek;
        case AVTransport::Action::Next:     return Action::Next;
        case AVTransport::Action::Previous: return Action::Previous;
        case AVTransport::Action::Record:   return Action::Record;
        default: throw std::logic_error("Invalid transport action");
    }
}

void MediaRenderer::throwOnUnknownConnectionId()
{
    if (m_ConnInfo.connectionId == ConnectionManager::UnknownConnectionId)
    {
        throw std::logic_error("No active renderer connection");
    }
}

}
