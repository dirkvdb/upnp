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

#include "upnp/upnpclientinterface.h"

namespace upnp
{

class Client : public IClient
{
public:
    Client();
    Client(const Client&) = delete;
    ~Client();

    Client& operator=(const Client&) = delete;
    
    virtual void initialize(const char* interfaceName = nullptr, int port = 0);
    virtual void destroy();
    virtual void reset();
    
    virtual void searchDevices(Device::Type type, int timeout) const;
    
    virtual void subscribeToService(const std::string& publisherUrl, int32_t& timeout, Upnp_SID subscriptionId) const;
    virtual void subscribeToService(const std::string& publisherUrl, int32_t timeout, Upnp_FunPtr callback, void* cookie) const;
    virtual void unsubscribeFromService(const Upnp_SID subscriptionId) const;
    
    virtual xml::Document sendAction(const Action& action) const;
    virtual xml::Document downloadXmlDocument(const std::string& url) const;
 
 private:
    static int upnpCallback(Upnp_EventType EventType, void* pEvent, void* pcookie);
    static void throwOnUPnPError(int errorCode);
    static const char* deviceTypeToString(Device::Type type);

    UpnpClient_Handle   m_Client;
};
    
}

#endif

