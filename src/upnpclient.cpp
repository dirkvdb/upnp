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

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{
    
Client::Client()
: m_Client(0)
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
        log::error("UpnpInit() Error: %d", rc);
        throw std::logic_error("Failed to initialize UPnP stack");
    }
    
    rc = UpnpRegisterClient(upnpCallback, this, &m_Client);
    if (UPNP_E_SUCCESS == rc)
    {
        UpnpSetMaxContentLength(1024 * 1024);
    }
    else if (UPNP_E_ALREADY_REGISTERED == rc)
    {
        log::warn("Control Point was already registered");
    }
    else if (UPNP_E_SUCCESS != rc )
    {
        log::error("Error registering Control Point: %d", rc);
        UpnpFinish();
        throw std::logic_error("Error registering Control Point");
    } 
    
    log::debug("Initialized: %s:%d", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void Client::destroy()
{
	if (m_Client)
    {
		UpnpUnRegisterClient(m_Client);
		UpnpFinish();
		m_Client = 0;

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

void Client::searchDevices(Device::Type type, int timeout) const
{
    log::debug("Send UPnP discovery");
    
    int rc = UpnpSearchAsync(m_Client, timeout, Device::deviceTypeToString(type).c_str(), this);
    if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error sending search request: %d", rc);
    }
}

std::string Client::subscribeToService(const std::string& publisherUrl, int32_t& timeout) const
{
    log::debug("Subscribe to service: %s", publisherUrl);
    
    Upnp_SID subscriptionId;
    int ret = UpnpSubscribe(m_Client, publisherUrl.c_str(), &timeout, subscriptionId);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device service");
    }
    
    return subscriptionId;
}

void Client::subscribeToService(const std::string& publisherUrl, int32_t timeout, Upnp_FunPtr callback, void* cookie) const
{
    log::debug("Subscribe to service: %s", publisherUrl);
    
    int ret = UpnpSubscribeAsync(m_Client, publisherUrl.c_str(), timeout, callback, cookie);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device service");
    }
}

void Client::unsubscribeFromService(const std::string& subscriptionId) const
{
    Upnp_SID id;
    if (subscriptionId.size() >= sizeof(id))
    {
        throw std::logic_error("Invalid subscription Id");
    }
    
    strcpy(id, subscriptionId.c_str());
    int ret = UpnpUnSubscribe(m_Client, id);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to unsubscribe from UPnP device service");
    }
}

xml::Document Client::sendAction(const Action& action) const
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: %s", action.getActionDocument().toString());
#endif

    IXML_Document* pDoc = nullptr;
    throwOnUPnPError(UpnpSendAction(m_Client, action.getUrl().c_str(), action.getServiceTypeUrn().c_str(), nullptr, action.getActionDocument(), &pDoc));
    
#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
    
    return xml::Document(pDoc);
}

xml::Document Client::downloadXmlDocument(const std::string& url) const
{
    IXML_Document* pDoc;
    int ret = UpnpDownloadXmlDoc(url.c_str(), &pDoc);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error(std::string("Error downloading xml document from ") + url);
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
            log::error("Error in Discovery Alive Callback: %d", pDiscEvent->ErrCode);
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
            log::error("Error in Discovery Bye Bye Callback: %d", pDiscEvent->ErrCode);
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

void Client::throwOnUPnPError(int32_t errorCode)
{
    if (errorCode != UPNP_E_SUCCESS)
    {
        throw UPnPException(errorCode);
    }
}



}
