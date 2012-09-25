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

MediaRenderer::MediaRenderer(Client& client)
: m_Client(client)
, m_ConnectionMgr(client)
, m_RenderingControl(client)
, m_Active(false)
{
}

std::shared_ptr<Device> MediaRenderer::getDevice()
{
    return m_Device;
}

void MediaRenderer::setDevice(const std::shared_ptr<Device>& device)
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

void MediaRenderer::increaseVolume(const ConnectionInfo& info, uint32_t percentage)
{
    m_RenderingControl.increaseVolume(info.connectionId, percentage);
}

void MediaRenderer::decreaseVolume(const ConnectionInfo& info, uint32_t percentage)
{
    m_RenderingControl.decreaseVolume(info.connectionId, percentage);
}

void MediaRenderer::activateEvents()
{
    if (!m_Active && m_Device)
    {
        m_RenderingControl.subscribe();

        if (m_AVtransport)
        {
            m_AVtransport->LastChangedEvent.connect(std::bind(&MediaRenderer::onLastChanged, this, _1), this);
            m_AVtransport->subscribe();
        }
        
        m_Active = true;
    }
}

void MediaRenderer::deactivateEvents()
{
    if (m_Active && m_Device)
    {
        m_RenderingControl.unsubscribe();
    
        if (m_AVtransport)
        {
            m_AVtransport->LastChangedEvent.disconnect(this);
            m_AVtransport->unsubscribe();
        }
        
        m_Active = false;
    }
}

bool MediaRenderer::isActionAvailable(Action action) const
{
    return m_AvailableActions.find(action) != m_AvailableActions.end();
}

void MediaRenderer::onLastChanged(const std::map<AVTransport::Variable, std::string>& vars)
{
    auto iter = vars.find(AVTransport::Variable::CurrentTrackURI);
    if (iter != vars.end())
    {
        log::info("Last changed:", iter->second);
    }
    
    iter = vars.find(AVTransport::Variable::CurrentTransportActions);
    if (iter != vars.end())
    {
        updateAvailableActions(iter->second);
    }
}

void MediaRenderer::updateAvailableActions(const std::string& actionList)
{
    auto actions = stringops::tokenize(actionList, ",");
    
    m_AvailableActions.clear();
    std::for_each(actions.begin(), actions.end(), [this] (const std::string& action) {
        try { m_AvailableActions.insert(transportActionToAction(AVTransport::actionFromString(action))); }
        catch (std::exception& e) { log::warn(e.what()); }
    });
    
    AvailableActionsChanged(m_AvailableActions);
}

MediaRenderer::Action MediaRenderer::transportActionToAction(AVTransport::Action action)
{
    switch (action) {
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

ConnectionManager& MediaRenderer::connectionManager()
{
    return m_ConnectionMgr;
}

}
