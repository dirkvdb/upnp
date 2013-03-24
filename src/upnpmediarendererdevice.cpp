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

#include "upnp/upnpmediarendererdevice.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

#include <sstream>

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

MediaRendererDevice::MediaRendererDevice(const std::string& udn, const std::string& descriptionXml, int32_t advertiseIntervalInSeconds, IAVTransport& av)
: m_RootDevice(udn, descriptionXml, advertiseIntervalInSeconds)
, m_ConnectionManager(m_RootDevice, *this)
, m_RenderingControl(m_RootDevice, *this)
, m_AVTransport(m_RootDevice, av)
{
    m_ConnectionManager.setVariable(ConnectionManager::Variable::SourceProtocolInfo, "");
    m_ConnectionManager.setVariable(ConnectionManager::Variable::SinkProtocolInfo, ""
        "http-get:*:audio/L16;rate=44100;channels=1:DLNA.ORG_PN=LPCM,"
        "http-get:*:audio/L16;rate=44100;channels=2:DLNA.ORG_PN=LPCM,"
        "http-get:*:audio/L16;rate=48000;channels=1:DLNA.ORG_PN=LPCM,"
        "http-get:*:audio/L16;rate=48000;channels=2:DLNA.ORG_PN=LPCM,"
        "http-get:*:audio/mpeg:DLNA.ORG_PN=MP3,"
        "http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMABASE,"
        "http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAFULL,"
        "http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO,"
        "http-get:*:audio/3gpp:DLNA.ORG_PN=AAC_ISO,"
        "http-get:*:audio/vnd.dlna.adts:DLNA.ORG_PN=AAC_ADTS_320,"
        "http-wavetunes:*:audio/x-ms-wma:*,"
        "http-get:*:audio/wav:*,"
        "http-get:*:audio/x-wav:*,"
        "http-get:*:audio/flac:*,"
        "http-get:*:audio/x-flac:*");
    
    m_RenderingControl.setVariable(RenderingControl::Variable::PresetNameList, "FactoryDefaults");
    m_RenderingControl.setInstanceVariable(0, RenderingControl::Variable::Volume, "50", "channel", RenderingControl::toString(RenderingControl::Channel::Master));
}

void MediaRendererDevice::start()
{
    m_RootDevice.ControlActionRequested.connect(std::bind(&MediaRendererDevice::onControlActionRequest, this, _1), this);
    m_RootDevice.EventSubscriptionRequested.connect(std::bind(&MediaRendererDevice::onEventSubscriptionRequest, this, _1), this);

    m_RootDevice.initialize();
}

void MediaRendererDevice::stop()
{
    m_RootDevice.ControlActionRequested.disconnect(this);
    m_RootDevice.EventSubscriptionRequested.disconnect(this);

    m_RootDevice.destroy();
}
    
void MediaRendererDevice::onEventSubscriptionRequest(Upnp_Subscription_Request* pRequest)
{
    log::debug("Renderer: event subscription request %s", pRequest->ServiceId);
    
    switch (serviceIdUrnStringToService(pRequest->ServiceId))
    {
    case ServiceType::AVTransport:              return m_RootDevice.acceptSubscription(pRequest->ServiceId, pRequest->Sid, m_AVTransport.getSubscriptionResponse());
    case ServiceType::RenderingControl:         return m_RootDevice.acceptSubscription(pRequest->ServiceId, pRequest->Sid, m_RenderingControl.getSubscriptionResponse());
    case ServiceType::ConnectionManager:        return m_RootDevice.acceptSubscription(pRequest->ServiceId, pRequest->Sid, m_ConnectionManager.getSubscriptionResponse());
    default:
        log::warn("Invalid event subscription request: %s", pRequest->ServiceId);
    }
}

void MediaRendererDevice::onControlActionRequest(Upnp_Action_Request* pRequest)
{
    //log::debug("Renderer: action request: %s", pRequest->ActionName);
    
    xml::Document requestDoc(pRequest->ActionRequest, xml::Document::NoOwnership);
    //log::debug(requestDoc.toString());
    
    switch (serviceIdUrnStringToService(pRequest->ServiceID))
    {
    case ServiceType::AVTransport:
        pRequest->ActionResult = m_AVTransport.onAction(pRequest->ActionName, requestDoc).getActionDocument();
        break;
    case ServiceType::RenderingControl:
        pRequest->ActionResult = m_RenderingControl.onAction(pRequest->ActionName, requestDoc).getActionDocument();
        break;
    case ServiceType::ConnectionManager:
        pRequest->ActionResult = m_ConnectionManager.onAction(pRequest->ActionName, requestDoc).getActionDocument();
        break;
    default:
        throw ServiceException("Invalid subscribtionId", 401);
    }
}

void MediaRendererDevice::selectPreset(uint32_t instanceId, const std::string& name)
{
}

void MediaRendererDevice::setVolume(uint32_t instanceId, upnp::RenderingControl::Channel channel, uint16_t value)
{
    log::info("Set volume: %s %d", upnp::RenderingControl::toString(channel), value);
    m_RenderingControl.setVolume(instanceId, channel, value);
}

void MediaRendererDevice::setMute(uint32_t instanceId, upnp::RenderingControl::Channel channel, bool enabled)
{
    log::info("Set mute: %s %d", upnp::RenderingControl::toString(channel), enabled);
    m_RenderingControl.setMute(instanceId, channel, enabled);
}

void MediaRendererDevice::prepareForConnection(const upnp::ProtocolInfo& protocolInfo, upnp::ConnectionManager::ConnectionInfo& info)
{
}

void MediaRendererDevice::connectionComplete(int32_t connectionId)
{
}

upnp::ConnectionManager::ConnectionInfo MediaRendererDevice::getCurrentConnectionInfo(int32_t connectionId)
{
    upnp::ConnectionManager::ConnectionInfo info;
    return info;
}

}
