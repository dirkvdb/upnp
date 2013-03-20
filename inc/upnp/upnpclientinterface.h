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

#ifndef UPNP_CLIENT_INTERFACE_H
#define UPNP_CLIENT_INTERFACE_H

#include <string>
#include <mutex>
#include <upnp/upnp.h>

#include "utils/subscriber.h"
#include "utils/signal.h"

#include "upnp/upnptypes.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpxmlutils.h"

namespace upnp
{
    
class IClient
{
public:
    virtual void initialize(const char* interfaceName = nullptr, int32_t port = 0) = 0;
    virtual void destroy() = 0;
    virtual void reset() = 0;
    
    virtual std::string getIpAddress() const = 0;
    virtual int32_t getPort() const = 0;
    virtual void searchDevices(Device::Type type, int timeout) const = 0;
    
    virtual std::string subscribeToService(const std::string& publisherUrl, int32_t& timeout) const = 0;
    virtual void subscribeToService(const std::string& publisherUrl, int32_t timeout, Upnp_FunPtr callback, void* cookie) const = 0;
    virtual void unsubscribeFromService(const std::string& subscriptionId) const = 0;
    
    virtual xml::Document sendAction(const Action& action) const = 0;
    virtual xml::Document downloadXmlDocument(const std::string& url) const = 0;
    
    utils::Signal<void(Upnp_Discovery*)> UPnPDeviceDiscoveredEvent;
    utils::Signal<void(const std::string&)> UPnPDeviceDissapearedEvent;
    utils::Signal<void(Upnp_Event*)> UPnPEventOccurredEvent;
};
    
}

#endif

