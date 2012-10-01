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

#include "utils/log.h"

#include <stdexcept>
#include <algorithm>

using namespace utils;

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
    
    int rc = UpnpInit2(interfaceName, port);
    if (UPNP_E_SUCCESS != rc && UPNP_E_INIT != rc)
    {
        UpnpFinish();
        log::error("UpnpInit() Error:", rc);
        throw std::logic_error("Failed to initialize UPnP stack");
    }
    
    rc = UpnpRegisterClient(upnpCallback, this, &m_Client);
    if (UPNP_E_SUCCESS == rc)
    {
        UpnpSetMaxContentLength(512 * 1024);
    }
    else if (UPNP_E_ALREADY_REGISTERED == rc)
    {
        log::warn("Control Point was already registered");
    }
    else if (UPNP_E_SUCCESS != rc )
    {
        log::error("Error registering Control Point: ", rc);
        UpnpFinish();
        throw std::logic_error("Error registering Control Point");
    } 
    
    log::debug("Initialized:", UpnpGetServerIpAddress(), ":", UpnpGetServerPort());
}

void Client::destroy()
{
    UpnpUnRegisterClient(m_Client);
    UpnpFinish();
    
    log::debug("Destroyed UPnP SDK");
}

void Client::reset()
{
    destroy();
    initialize();
}

void Client::searchDevices(Device::Type type, int timeout) const
{
    log::debug("Send UPnP discovery");
    
    int rc = UpnpSearchAsync(m_Client, timeout, Device::deviceTypeToString(type).c_str(), this);
    if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error sending search request:", rc);
    }
}

void Client::subscribeToService(const std::string& publisherUrl, int32_t& timeout, Upnp_SID subscriptionId) const
{
    log::debug("Subscribe to service:", publisherUrl);
    
    int ret = UpnpSubscribe(m_Client, publisherUrl.c_str(), &timeout, subscriptionId);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device service");
    }
}

void Client::subscribeToService(const std::string& publisherUrl, int32_t timeout, Upnp_FunPtr callback, void* cookie) const
{
    log::debug("Subscribe to service:", publisherUrl);
    
    int ret = UpnpSubscribeAsync(m_Client, publisherUrl.c_str(), timeout, callback, cookie);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device service");
    }
}

void Client::unsubscribeFromService(const Upnp_SID subscriptionId) const
{
    int ret = UpnpUnSubscribe(m_Client, subscriptionId);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to unsubscribe from UPnP device service");
    }
}

xml::Document Client::sendAction(const Action& action) const
{
    xml::Document result;
    throwOnUPnPError(UpnpSendAction(m_Client, action.getUrl().c_str(), action.getServiceTypeUrn().c_str(), nullptr, action.getActionDocument(), &result));
    
    return result;
}

xml::Document Client::downloadXmlDocument(const std::string& url) const
{
    xml::Document doc;
    int ret = UpnpDownloadXmlDoc(url.c_str(), &doc);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error(std::string("Error downloading xml document from ") + url);
    }
    
    return doc;
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
            log::error("Error in Discovery Alive Callback:", pDiscEvent->ErrCode);
        }
        else if (pDiscEvent->DeviceId && pDiscEvent->DeviceType && pDiscEvent->Location)
        {
            pClient->UPnPDeviceDiscoveredEvent(pDiscEvent);
        }
        break;
    }
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
        struct Upnp_Discovery* pDiscEvent = reinterpret_cast<struct Upnp_Discovery*>(pEvent);
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Discovery Bye Bye Callback:", pDiscEvent->ErrCode);
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
        throw errorCode;
    }
}



}
