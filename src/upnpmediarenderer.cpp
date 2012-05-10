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

#include <sstream>

using namespace utils;
using namespace std::placeholders;


namespace upnp
{

MediaRenderer::MediaRenderer(Client& client)
: m_Client(client)
, m_ConnectionMgr(client)
, m_RenderingControl(client)
{
}

std::shared_ptr<Device> MediaRenderer::getDevice()
{
    return m_Device;
}

void MediaRenderer::setDevice(std::shared_ptr<Device> device)
{
    if (m_Device)
    {
        m_AVtransport->LastChangedEvent.disconnect(this);
    }

    m_Device = device;
    m_ConnectionMgr.setDevice(device);
    m_RenderingControl.setDevice(device);
    
    if (m_Device->implementsService(Service::Type::AVTransport))
    {
        m_AVtransport.reset(new AVTransport(m_Client));
        m_AVtransport->LastChangedEvent.connect(std::bind(&MediaRenderer::onLastChanged, this, _1), this);
        m_AVtransport->setDevice(device);
    }
    
    m_ProtocolInfo = m_ConnectionMgr.getProtocolInfo();
}

bool MediaRenderer::supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const
{
    if (!m_Device)
    {
        throw std::logic_error("No UPnP renderer selected");
    }

    for (auto& res : item->getResources())
    {
        for (auto& info : m_ProtocolInfo)
        {
            if (info.isCompatibleWith(res.getProtocolInfo()))
            {
                suggestedResource = res;
                return true;
            }
        }
    }
    
    return false;
}

std::string MediaRenderer::getPeerConnectionId() const
{
    std::stringstream ss;
    ss << m_Device->m_UDN << "/" << m_Device->m_Services[Service::ConnectionManager].m_Id;
    
    return ss.str();
}

void MediaRenderer::setTransportItem(const ConnectionInfo& info, Resource& resource)
{
    if (m_AVtransport)
    {
        m_AVtransport->setAVTransportURI(info.connectionId, resource.getUrl(), "");
    }
}

void MediaRenderer::play(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->play(info.connectionId, "1");
    }
}

void MediaRenderer::stop(const ConnectionInfo& info)
{
    if (m_AVtransport)
    {
        m_AVtransport->stop(info.connectionId);
    }
}

void MediaRenderer::activateEvents()
{
    if (m_AVtransport)
    {
        m_AVtransport->subscribe();
    }
}

void MediaRenderer::deactivateEvents()
{
    if (m_AVtransport)
    {
        m_AVtransport->unsubscribe();
    }
}

void MediaRenderer::onLastChanged(const std::map<AVTransport::Variable, std::string>& vars)
{
    auto iter = vars.find(AVTransport::Variable::CurrentTrackURI);
    if (iter != vars.end())
        log::info("Last changed:", iter->second);
}

ConnectionManager& MediaRenderer::connectionManager()
{
    return m_ConnectionMgr;
}

}
