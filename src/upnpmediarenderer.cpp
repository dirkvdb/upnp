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
            m_AVtransport.reset(new AVTransport(m_Client));
            m_AVtransport->setDevice(device);
        }
        
        m_ProtocolInfo = m_ConnectionMgr.getProtocolInfo();
        
        activateEvents();
    }
    catch (std::exception& e)
    {
        throw std::logic_error(std::string("Failed to set renderer device:") + e.what());
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

std::string MediaRenderer::getPeerConnectionId() const
{
    std::stringstream ss;
    ss << m_Device->m_UDN << "/" << m_Device->m_Services[ServiceType::ConnectionManager].m_Id;
    
    return ss.str();
}

void MediaRenderer::setTransportItem(const ConnectionInfo& info, Resource& resource)
{
    if (m_AVtransport)
    {
        m_AVtransport->setAVTransportURI(info.connectionId, resource.getUrl());
    }
}

void MediaRenderer::play(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->play(info.connectionId);
    }
}

void MediaRenderer::pause(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->pause(info.connectionId);
    }
}

void MediaRenderer::stop(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->stop(info.connectionId);
    }
}

void MediaRenderer::next(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->next(info.connectionId);
    }
}

void MediaRenderer::previous(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->previous(info.connectionId);
    }
}

std::string MediaRenderer::getCurrentTrackURI() const
{
    auto iter = m_AvTransportInfo.find(AVTransportVariable::CurrentTrackURI);
    return iter == m_AvTransportInfo.end() ? "" : iter->second;
}

std::string MediaRenderer::getCurrentTrackDuration() const
{
    auto iter = m_AvTransportInfo.find(AVTransportVariable::CurrentTrackDuration);
    return iter == m_AvTransportInfo.end() ? "" : iter->second;
}

Item MediaRenderer::getCurrentTrackInfo() const
{
    Item item;

    auto iter = m_AvTransportInfo.find(AVTransportVariable::CurrentTrackDuration);
    if (iter != m_AvTransportInfo.end())
    {
        log::warn("Meta parsing not implemented");
    }
    
    return item;
}

bool MediaRenderer::isActionAvailable(Action action) const
{
    return m_AvailableActions.find(action) != m_AvailableActions.end();
}

void MediaRenderer::setVolume(const ConnectionInfo& info, uint32_t value)
{
    m_RenderingControl.setVolume(info.connectionId, value);
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
        m_RenderingControl.StateVariableEvent.disconnect(this);
        m_RenderingControl.unsubscribe();
    
        if (m_AVtransport)
        {
            m_AVtransport->StateVariableEvent.disconnect(this);
            m_AVtransport->unsubscribe();
        }
        
        m_Active = false;
    }
}

void MediaRenderer::calculateAvailableActions()
{
    auto iter = m_AvTransportInfo.find(AVTransportVariable::CurrentTransportActions);
    if (iter == m_AvTransportInfo.end())
    {
        // no action info provided, allow everything
        m_AvailableActions =  { Action::Play, Action::Stop, Action::Pause, Action::Seek,
                                Action::Next, Action::Previous, Action::Record };
        return;
    }
    
    auto actionsStrings = stringops::tokenize(iter->second, ",");
    std::for_each(actionsStrings.begin(), actionsStrings.end(), [&] (const std::string& action) {
        try { m_AvailableActions.insert(transportActionToAction(m_AVtransport->actionFromString(action))); }
        catch (std::exception& e) { log::warn(e.what()); }
    });
}

void MediaRenderer::onRenderingControlLastChangeEvent(const std::map<RenderingControlVariable, std::string>& vars)
{
    auto iter = vars.find(RenderingControlVariable::Volume);
    if (iter != vars.end())
    {
        m_CurrentVolume = utils::stringops::toNumeric<uint32_t>(iter->second);
        VolumeChanged(m_CurrentVolume);
    }
}

void MediaRenderer::onAVTransportLastChangeEvent(const std::map<AVTransportVariable, std::string>& vars)
{
    m_AvTransportInfo = vars;
    calculateAvailableActions();
    MediaInfoChanged();
}

MediaRenderer::Action MediaRenderer::transportActionToAction(AVTransportAction action)
{
    switch (action) {
        case AVTransportAction::Play:     return Action::Play;
        case AVTransportAction::Stop:     return Action::Stop;
        case AVTransportAction::Pause:    return Action::Pause;
        case AVTransportAction::Seek:     return Action::Seek;
        case AVTransportAction::Next:     return Action::Next;
        case AVTransportAction::Previous: return Action::Previous;
        case AVTransportAction::Record:   return Action::Record;
        default: throw std::logic_error("Invalid transport action");
    }
}

ConnectionManager& MediaRenderer::connectionManager()
{
    return m_ConnectionMgr;
}

}
