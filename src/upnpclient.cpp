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
    log::debug(this, " Client:", &UPnPDeviceDiscoveredEvent);
}

Client::~Client()
{
    destroy();
}

void Client::initialize()
{
    log::debug("Initializing UPnP SDK");
    
    int rc = UpnpInit(nullptr, 0);
    if (UPNP_E_SUCCESS != rc && UPNP_E_INIT != rc)
    {
        UpnpFinish();
        log::error("UpnpInit() Error:", rc);
        throw std::logic_error("Failed to initialise UPnP stack");
    }
    
    rc = UpnpRegisterClient(upnpCallback, this, &m_Client);
    if (UPNP_E_SUCCESS == rc)
    {
        UpnpSetMaxContentLength(128 * 1024);
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
}

void Client::destroy()
{
    UpnpUnRegisterClient(m_Client);
    UpnpFinish();
    
    log::info("Destroyed UPnP SDK");
}

void Client::reset()
{
    destroy();
    initialize();
}

int Client::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    Client* pClient = reinterpret_cast<Client*>(pCookie);
    if (!pClient)
    {
        log::error("Empty cookie:", eventType);
        return 0;
    }
    
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
            Discovery disc;
            disc.udn            = pDiscEvent->DeviceId;
            disc.deviceType     = pDiscEvent->DeviceType;
            disc.location       = pDiscEvent->Location;
            
            pClient->UPnPDeviceDiscoveredEvent(disc);
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

}
