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

#include <string>
#include <upnp/upnp.h>

#include "utils/types.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpactionresponse.h"

namespace upnp
{
    
struct ActionRequest
{
    std::string     actionName;
    xml::Document   request;
    xml::Document   result;
    int32_t         errorCode;          // fill this in when something goes wrong
    std::string     errorString;        // fill this in when something goes wrong
};

class RootDevice
{
public:
    RootDevice(const std::string& descriptionXml, int32_t advertiseIntervalInSeconds);
    ~RootDevice();
    
    virtual void onEventSubscriptionRequest(const std::string& udn, const std::string& serviceId, const std::string& subscriptionId) = 0;
    virtual void onControlActionRequest(const std::string& udn, const std::string& serviceId, ActionRequest& request) = 0;

protected:
    void AcceptSubscription(const std::string& udn, const std::string& serviceId, const std::string& subscriptionId, const std::map<std::string, std::string>& vars);
    
private:
    static int upnpCallback(Upnp_EventType eventType, void* event, void* cookie);
    
    UpnpDevice_Handle   m_device;
};

}

#endif
