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

#ifndef UPNP_MEDIA_RENDERER_DEVICE_H
#define UPNP_MEDIA_RENDERER_DEVICE_H

#include <memory>

#include "upnp/upnprootdevice.h"
#include "upnp/upnpdeviceservice.h"
#include "upnp/upnpactionresponse.h"
#include "upnp/upnprenderingcontrolservice.h"
#include "upnp/upnpavtransportservice.h"
#include "upnp/upnpconnectionmanagerservice.h"

namespace upnp
{

class MediaRendererDevice : public IConnectionManager
                          , public IRenderingControl
{
public:
    MediaRendererDevice(const std::string& udn, const std::string& descriptionXml, int32_t advertiseIntervalInSeconds, IAVTransport& av);
    MediaRendererDevice(const MediaRendererDevice&) = delete;
    
    void start();
    void stop();
    
    /* Not for public use */
    virtual void prepareForConnection(const ProtocolInfo& protocolInfo, ConnectionManager::ConnectionInfo& info);
    virtual void connectionComplete(int32_t connectionId);
    virtual ConnectionManager::ConnectionInfo getCurrentConnectionInfo(int32_t connectionId);
    
    virtual void selectPreset(uint32_t instanceId, const std::string& name);
    virtual void setVolume(uint32_t instanceId, upnp::RenderingControl::Channel channel, uint16_t value);
    virtual void setMute(uint32_t instanceId, upnp::RenderingControl::Channel channel, bool enabled);
    
private:
    void onEventSubscriptionRequest(Upnp_Subscription_Request* pRequest);
    void onControlActionRequest(Upnp_Action_Request* pRequest);


    mutable std::mutex                  m_Mutex;
    
    RootDevice                          m_RootDevice;
    ConnectionManager::Service          m_ConnectionManager;
    RenderingControl::Service           m_RenderingControl;
    AVTransport::Service                m_AVTransport;
};

}

#endif
