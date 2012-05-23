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

#ifndef UPNP_CLIENT_H
#define UPNP_CLIENT_H

#include <string>
#include <mutex>
#include <upnp/upnp.h>

#include "utils/subscriber.h"
#include "utils/signal.h"


namespace upnp
{
    
class Client
{
public:
    Client();
    Client(const Client&) = delete;
    ~Client();

    Client& operator=(const Client&) = delete;
    
    void initialize(const char* interfaceName = nullptr, int port = 0);
    void destroy();
    
    operator UpnpClient_Handle() const { return m_Client; }
    
    void reset();
    
    utils::Signal<void(Upnp_Discovery*)> UPnPDeviceDiscoveredEvent;
    utils::Signal<void(const std::string&)> UPnPDeviceDissapearedEvent;
    utils::Signal<void(Upnp_Event*)> UPnPEventOccurredEvent;    
    
private:
    static int upnpCallback(Upnp_EventType EventType, void* pEvent, void* pcookie);

    UpnpClient_Handle                   		m_Client;
};
    
}

#endif

