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

#include "upnp.client.h"

#include "utils/log.h"
#include "upnp/upnputils.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

Client2::Client2(uv::Loop& loop)
: m_loop(loop)
{
}

void Client2::initialize(const std::string& interfaceName, int32_t port)
{
    log::debug("Initializing UPnP SDK");

    auto upnp   = std::make_unique<UpnpInitialization>(interfaceName, port);
    m_client    = std::make_unique<ClientHandle>(upnpCallback, this);
    m_upnp = std::move(upnp);

    UpnpSetMaxContentLength(1024 * 1024);

    log::debug("Initialized: {}:{}", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void Client2::destroy()
{
    m_client.reset();
    m_upnp.reset();
    log::debug("Destroyed UPnP SDK");
}

void Client2::reset()
{
    destroy();
    initialize();
}

std::string Client2::getIpAddress() const
{
    return UpnpGetServerIpAddress();
}

int32_t Client2::getPort() const
{
    return UpnpGetServerPort();
}

void Client2::searchDevicesOfType(DeviceType type, int32_t timeout) const
{
    log::debug("Send UPnP discovery");
    handleUPnPResult(UpnpSearchAsync(*m_client, timeout, Device::deviceTypeToString(type).c_str(), this), "Error sending search request");
}

void Client2::searchAllDevices(int32_t timeout) const
{
    log::debug("Send UPnP discovery");
    handleUPnPResult(UpnpSearchAsync(*m_client, timeout, "ssdp:all", this), "Error sending search request");
}

std::string Client2::subscribeToService(const std::string& publisherUrl, int32_t& timeout) const
{
    log::debug("Subscribe to service: {}", publisherUrl);

    Upnp_SID subscriptionId;
    handleUPnPResult(UpnpSubscribe(*m_client, publisherUrl.c_str(), &timeout, subscriptionId), "Failed to subscribe to UPnP device service");
    return subscriptionId;
}

void Client2::unsubscribeFromService(const std::string& subscriptionId) const
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

void Client2::subscribeToService(const std::string& publisherUrl, int32_t timeout, const std::shared_ptr<IServiceSubscriber>& sub) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    log::debug("Subscribe to service: {}", publisherUrl);
    handleUPnPResult(UpnpSubscribeAsync(*m_client, publisherUrl.c_str(), timeout, &Client2::upnpServiceCallback, sub.get()), "Failed to subscribe to UPnP device service");
    m_serviceSubscriptions.insert(std::make_pair(sub.get(), std::weak_ptr<IServiceSubscriber>(sub)));
}

void Client2::unsubscribeFromService(const std::shared_ptr<IServiceSubscriber>& sub) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_serviceSubscriptions.erase(sub.get());
    unsubscribeFromService(sub->getSubscriptionId());
}

xml::Document Client2::sendAction(const Action& action) const
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    IXML_Document* pDoc = nullptr;
    handleUPnPResult(UpnpSendAction(*m_client, action.getUrl().c_str(), action.getServiceTypeUrn().c_str(), nullptr, action.getActionDocument(), &pDoc));

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif

    return xml::Document(pDoc);
}

xml::Document Client2::downloadXmlDocument(const std::string& url) const
{
    IXML_Document* pDoc;
    handleUPnPResult(UpnpDownloadXmlDoc(url.c_str(), &pDoc), "Error downloading xml document from {}", url);
    return xml::Document(pDoc);
}

int Client2::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
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

int Client2::upnpServiceCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
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
