//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#ifndef UPNP_ROOT_DEVICE_H
#define UPNP_ROOT_DEVICE_H

#include "utils/types.h"
#include "utils/timerthread.h"
#include "upnp/upnptypes.h"
#include "upnp/upnprootdeviceinterface.h"

namespace upnp
{
    
class RootDevice : public IRootDevice
{
public:
    RootDevice(const std::string& udn, const std::string& descriptionXml, int32_t advertiseIntervalInSeconds);
    ~RootDevice();
    
    virtual void initialize();
    virtual void destroy();
    
    virtual std::string getUniqueDeviceName();
    virtual void acceptSubscription(const std::string& serviceId, const std::string& subscriptionId, const xml::Document& response);
    virtual void notifyEvent(const std::string& serviceId, const xml::Document& event);
    
private:
    static int upnpCallback(Upnp_EventType eventType, void* event, void* cookie);
    
    UpnpDevice_Handle   m_Device;
    std::string         m_Udn;
    std::string         m_DescriptionXml;
    int32_t             m_AdvertiseInterval;
    utils::TimerThread  m_AdvertiseThread;
};

}

#endif
