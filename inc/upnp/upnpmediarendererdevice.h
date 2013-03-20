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

class MediaRendererDevice : public RootDevice
{
public:
    MediaRendererDevice(const std::string& descriptionXml, int32_t advertiseIntervalInSeconds, IConnectionManager& cm, IRenderingControl& rc);
    MediaRendererDevice(const MediaRendererDevice&) = delete;
    
    virtual void onEventSubscriptionRequest(const std::string& udn, const std::string& serviceId, const std::string& subscriptionId);
    virtual void onControlActionRequest(const std::string& udn, const std::string& serviceId, ActionRequest& request);
    
private:
    mutable std::mutex                                      m_Mutex;
    std::map<ServiceType, std::unique_ptr<DeviceService>>   m_Services;
};

}

#endif
