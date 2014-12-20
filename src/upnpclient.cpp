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

#include "upnp/upnpclient.h"

#include <upnp/upnpconfig.h>

#include "utils/log.h"
#include "upnp/upnputils.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{
    
Client::Client()
: m_client(0)
{
}

Client::~Client()
{
    destroy();
}

void Client::initialize(const char* interfaceName, int port)
{
    log::debug("Initializing UPnP SDK");
    
#ifdef UPNP_ENABLE_IPV6
    int rc = UpnpInit2(interfaceName, port);
#else
    int rc = UpnpInit(interfaceName, port);
#endif
    if (UPNP_E_SUCCESS != rc && UPNP_E_INIT != rc)
    {
        UpnpFinish();
        log::error("UpnpInit() Error: {}", rc);
        throw Exception(rc, "Failed to initialize UPnP stack");
    }
    
    rc = UpnpRegisterClient(upnpCallback, this, &m_client);
    if (UPNP_E_SUCCESS == rc)
    {
        UpnpSetMaxContentLength(1024 * 1024);
    }
    else if (UPNP_E_ALREADY_REGISTERED == rc)
    {
        log::warn("Control Point was already registered");
    }
    else if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error registering Control Point: {}", rc);
        UpnpFinish();
        throw Exception(rc, "Error registering Control Point");
    } 
    
    log::debug("Initialized: {}:{}", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void Client::destroy()
{
	if (m_client)
    {
		UpnpUnRegisterClient(m_client);
		UpnpFinish();
		m_client = 0;

		log::debug("Destroyed UPnP SDK");
    }
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
    
    int rc = UpnpSearchAsync(m_client, timeout, Device::deviceTypeToString(type).c_str(), this);
    if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error sending search request: {}", rc);
    }
}

void Client::searchAllDevices(int32_t timeout) const
{
    log::debug("Send UPnP discovery");
    
    int rc = UpnpSearchAsync(m_client, timeout, "ssdp:all", this);
    if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error sending search request: {}", rc);
    }
}

std::string Client::subscribeToService(const std::string& publisherUrl, int32_t& timeout) const
{
    log::debug("Subscribe to service: {}", publisherUrl);
    
    Upnp_SID subscriptionId;
    int rc = UpnpSubscribe(m_client, publisherUrl.c_str(), &timeout, subscriptionId);
    if (rc != UPNP_E_SUCCESS)
    {
        throw Exception(rc, "Failed to subscribe to UPnP device service");
    }
    
    return subscriptionId;
}

void Client::subscribeToService(const std::string& publisherUrl, int32_t timeout, Upnp_FunPtr callback, void* cookie) const
{
    log::debug("Subscribe to service: {}", publisherUrl);
    
    int rc = UpnpSubscribeAsync(m_client, publisherUrl.c_str(), timeout, callback, cookie);
    if (rc != UPNP_E_SUCCESS)
    {
        throw Exception(rc, "Failed to subscribe to UPnP device service");
    }
}

void Client::unsubscribeFromService(const std::string& subscriptionId) const
{
    Upnp_SID id;
    if (subscriptionId.size() >= sizeof(id))
    {
        throw Exception("Invalid subscription Id");
    }
    
    strcpy(id, subscriptionId.c_str());
    int rc = UpnpUnSubscribe(m_client, id);
    if (rc != UPNP_E_SUCCESS)
    {
        throw Exception(rc, "Failed to unsubscribe from UPnP device service");
    }
}

xml::Document Client::sendAction(const Action& action) const
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    IXML_Document* pDoc = nullptr;
    handleUPnPResult(UpnpSendAction(m_client, action.getUrl().c_str(), action.getServiceTypeUrn().c_str(), nullptr, action.getActionDocument(), &pDoc));

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
    
    return xml::Document(pDoc);
}

xml::Document Client::downloadXmlDocument(const std::string& url) const
{
    IXML_Document* pDoc;
    int rc = UpnpDownloadXmlDoc(url.c_str(), &pDoc);
    if (rc != UPNP_E_SUCCESS)
    {
        throw Exception(rc, "Error downloading xml document from {}", url);
    }
    
    return xml::Document(pDoc);
}

int Client::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    Client* pClient = reinterpret_cast<Client*>(pCookie);
    
    switch (eventType)
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
        struct Upnp_Discovery* pDiscEvent = reinterpret_cast<struct Upnp_Discovery*>(pEvent);
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Discovery Alive Callback: {}", pDiscEvent->ErrCode);
        }
        else if (pDiscEvent->DeviceId && pDiscEvent->DeviceType && pDiscEvent->Location)
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
        struct Upnp_Discovery* pDiscEvent = reinterpret_cast<struct Upnp_Discovery*>(pEvent);
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
    {
        pClient->UPnPEventOccurredEvent(reinterpret_cast<Upnp_Event*>(pEvent));
        break;
    }
    default:
        break;
    }
    
    return 0;
}

}
