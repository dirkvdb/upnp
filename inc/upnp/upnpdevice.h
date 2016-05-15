//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H

#include <string>
#include <map>
#include <chrono>
#include <cinttypes>

#include "upnp/upnp.types.h"

namespace upnp
{

static const char* MediaServerDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* MediaRendererDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaRenderer:1";
static const char* InternetGatewayDeviceTypeUrn = "urn:schemas-upnp-org:device:InternetGatewayDevice:1";

class Service
{
public:
    ServiceType     m_type;
    std::string     m_id;
    std::string     m_scpdUrl;
    std::string     m_controlURL;
    std::string     m_eventSubscriptionURL;
    std::string     m_eventSubscriptionID;
};

class Device
{
public:
    bool operator==(const Device& otherDevice) const { return m_udn == otherDevice.m_udn; }

    bool implementsService(ServiceType type) const { return m_services.find(type) != m_services.end(); }

    DeviceType      m_type;
    std::string     m_userDefinedName;
    std::string     m_friendlyName;
    std::string     m_udn;
    std::string     m_baseURL;
    std::string     m_relURL;
    std::string     m_presURL;
    std::string     m_location;
    std::string     m_containerId;

    std::chrono::system_clock::time_point   m_timeoutTime;

    std::map<ServiceType, Service>    m_services;

    static const char* deviceTypeToString(DeviceType type)
    {
        switch (type)
        {
            case DeviceType::MediaServer:
                return MediaServerDeviceTypeUrn;
            case DeviceType::MediaRenderer:
                return MediaRendererDeviceTypeUrn;
            case DeviceType::InternetGateway:
                return InternetGatewayDeviceTypeUrn;
            default:
                throw std::invalid_argument("Invalid device type encountered");
        }
    }

    static DeviceType stringToDeviceType(const std::string& type)
    {
        if (type == MediaServerDeviceTypeUrn)       { return DeviceType::MediaServer; }
        if (type == MediaRendererDeviceTypeUrn)     { return DeviceType::MediaRenderer; }
        if (type == InternetGatewayDeviceTypeUrn)   { return DeviceType::InternetGateway; }

        return DeviceType::Unknown;
    }
};

class IDeviceSubscriber
{
public:
    enum DeviceEvent
    {
        deviceDiscovered,
        deviceDissapeared
    };

    virtual ~IDeviceSubscriber() = default;
    virtual void onUPnPDeviceEvent(const Device& item, DeviceEvent event) = 0;
};

}

#endif
