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

#include "upnpclient.h"

#include <upnpconfig.h>

#include "utils/log.h"
#include "upnp/upnputils.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <upnp.h>

using namespace utils;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

std::map<IServiceSubscriber*, std::weak_ptr<IServiceSubscriber>> Client::m_serviceSubscriptions;
std::mutex Client::m_mutex;

class Client::UpnpInitialization
{
public:
    UpnpInitialization(const char* interfaceName, int port)
    {
    #ifdef UPNP_ENABLE_IPV6
        handleUPnPResult(UpnpInit2(interfaceName, port), "Failed to initialize UPnP stack");
    #else
        handleUPnPResult(UpnpInit(interfaceName, port), "Failed to initialize UPnP stack");
    #endif
    }

    ~UpnpInitialization()
    {
        UpnpFinish();
    }
};

class Client::ClientHandle
{
public:
    ClientHandle(Upnp_FunPtr cb, const void* cookie)
    {
        handleUPnPResult(UpnpRegisterClient(cb, cookie, &m_handle), "Error registering Control Point");
    }

    ~ClientHandle()
    {
        UpnpUnRegisterClient(m_handle);
    }

    operator UpnpClient_Handle() const
    {
        return m_handle;
    }

private:
    UpnpClient_Handle   m_handle;
};

Client::Client() = default;

Client::~Client()
{
    destroy();
}

void Client::initialize(const char* interfaceName, int port)
{
    log::debug("Initializing UPnP SDK");

    auto upnp   = std::make_unique<UpnpInitialization>(interfaceName, port);
    m_client    = std::make_unique<ClientHandle>(upnpCallback, this);
    m_upnp = std::move(upnp);

    UpnpSetMaxContentLength(1024 * 1024);

    log::debug("Initialized: {}:{}", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void Client::destroy()
{
    m_client.reset();
    m_upnp.reset();
    log::debug("Destroyed UPnP SDK");
}

void Client::reset()
{
    destroy();
    initialize();
}

std::string Client::getIpAddress() const
{
    return UpnpGetServerIpAddress();
}

int32_t Client::getPort() const
{
    return UpnpGetServerPort();
}

void Client::searchDevicesOfType(DeviceType type, int32_t timeout) const
{
    log::debug("Send UPnP discovery");
    handleUPnPResult(UpnpSearchAsync(*m_client, timeout, deviceTypeToString(type), this), "Error sending search request");
}

void Client::searchAllDevices(int32_t timeout) const
{
    log::debug("Send UPnP discovery");
    handleUPnPResult(UpnpSearchAsync(*m_client, timeout, "ssdp:all", this), "Error sending search request");
}

std::string Client::subscribeToService(const std::string& publisherUrl, int32_t& timeout) const
{
    log::debug("Subscribe to service: {}", publisherUrl);

    Upnp_SID subscriptionId;
    handleUPnPResult(UpnpSubscribe(*m_client, publisherUrl.c_str(), &timeout, subscriptionId), "Failed to subscribe to UPnP device service");
    return subscriptionId;
}

void Client::unsubscribeFromService(const std::string& subscriptionId) const
{
    Upnp_SID id;
    if (sizeof(id) >= (subscriptionId.size() + 1))
    {
        strcpy(id, subscriptionId.c_str());
    }
    else
    {
        throw Exception("Invalid subscription Id");
    }

    handleUPnPResult(UpnpUnSubscribe(*m_client, id), "Failed to unsubscribe from UPnP device service");
}

void Client::subscribeToService(const std::string& publisherUrl, int32_t timeout, const std::shared_ptr<IServiceSubscriber>& sub) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    log::debug("Subscribe to service: {}", publisherUrl);
    handleUPnPResult(UpnpSubscribeAsync(*m_client, publisherUrl.c_str(), timeout, &Client::upnpServiceCallback, sub.get()), "Failed to subscribe to UPnP device service");
    m_serviceSubscriptions.insert(std::make_pair(sub.get(), std::weak_ptr<IServiceSubscriber>(sub)));
}

void Client::unsubscribeFromService(const std::shared_ptr<IServiceSubscriber>& sub) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_serviceSubscriptions.erase(sub.get());
    unsubscribeFromService(sub->getSubscriptionId());
}

xml::Document Client::downloadXmlDocument(const std::string& url) const
{
    IXML_Document* pDoc;
    handleUPnPResult(UpnpDownloadXmlDoc(url.c_str(), &pDoc), "Error downloading xml document from {}", url);
    return xml::Document(pDoc);
}

int Client::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    auto pClient = reinterpret_cast<Client*>(pCookie);

    switch (eventType)
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
        auto pDiscEvent = reinterpret_cast<struct Upnp_Discovery*>(pEvent);
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Discovery Alive Callback: {}", pDiscEvent->ErrCode);
        }
        else
        {
            DeviceDiscoverInfo info;
            info.deviceId       = pDiscEvent->DeviceId;
            info.deviceType     = pDiscEvent->DeviceType;
            info.expirationTime = pDiscEvent->Expires;
            info.location       = pDiscEvent->Location;
            info.serviceType    = pDiscEvent->ServiceType;
            info.serviceVersion = pDiscEvent->ServiceVer;

            pClient->UPnPDeviceDiscoveredEvent(info);
        }
        break;
    }
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
        auto pDiscEvent = reinterpret_cast<struct Upnp_Discovery*>(pEvent);
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Discovery Bye Bye Callback: {}", pDiscEvent->ErrCode);
        }
        else
        {
            pClient->UPnPDeviceDissapearedEvent(pDiscEvent->DeviceId);
        }
        break;
    }
    case UPNP_EVENT_RECEIVED:
        pClient->UPnPEventOccurredEvent(reinterpret_cast<Upnp_Event*>(pEvent));
        break;
    default:
        break;
    }

    return 0;
}

int Client::upnpServiceCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        auto sub = m_serviceSubscriptions.at(reinterpret_cast<IServiceSubscriber*>(pCookie)).lock();
        if (sub)
        {
            sub->onServiceEvent(eventType, pEvent);
        }
    }
    catch (std::out_of_range&) {}

    // auto iter = m_serviceSubscriptions.find(subPtr);
    // if (iter != m_serviceSubscriptions.end())
    // {
    //     auto sub = iter->second.lock();
    //     if (sub)
    //     {
    //         sub->onServiceEvent(eventType, pEvent);
    //     }
    // }

    return 0;
}

}
