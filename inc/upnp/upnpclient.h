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
    virtual ~Client();

    Client& operator=(const Client&) = delete;
    
    virtual void initialize(const char* interfaceName = nullptr, int port = 0) override;
    virtual void destroy() override;
    virtual void reset() override;
    
    virtual std::string getIpAddress() const override;
    virtual int32_t getPort() const override;
    
    virtual void searchDevicesOfType(DeviceType type, int32_t timeout) const override;
    virtual void searchAllDevices(int32_t timeout) const override;
    
    virtual std::string subscribeToService(const std::string& publisherUrl, int32_t& timeout) const override;
    virtual void subscribeToService(const std::string& publisherUrl, int32_t timeout, const std::shared_ptr<IServiceSubscriber>& sub) const override;
    virtual void unsubscribeFromService(const std::string& subscriptionId) const override;
    
    virtual xml::Document sendAction(const Action& action) const override;
    virtual xml::Document downloadXmlDocument(const std::string& url) const override;
 
 private:
    static int upnpCallback(Upnp_EventType EventType, void* pEvent, void* pcookie);
    static int upnpServiceCallback(Upnp_EventType EventType, void* pEvent, void* pcookie);
    static const char* deviceTypeToString(DeviceType type);

    UpnpClient_Handle   m_client;
    static std::mutex   m_mutex;
    static std::map<IServiceSubscriber*, std::weak_ptr<IServiceSubscriber>> m_serviceSubscriptions;
};
    
}

#endif

