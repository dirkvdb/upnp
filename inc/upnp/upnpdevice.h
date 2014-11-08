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

#include "utils/types.h"
#include "upnp/upnptypes.h"

namespace upnp
{

namespace
{
    static const char* MediaServerDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaServer:1";
    static const char* MediaRendererDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaRenderer:1";
}

class Service
{
public:
    ServiceType     m_Type;
    std::string     m_Id;
    std::string     m_SCPDUrl;
    std::string     m_ControlURL;
    std::string     m_EventSubscriptionURL;
    std::string     m_EventSubscriptionID;
};

class Device
{
public:
    bool operator==(const Device& otherDevice) const { return m_UDN == otherDevice.m_UDN; }
    
    bool implementsService(ServiceType type) const { return m_Services.find(type) != m_Services.end(); }

    DeviceType      m_Type;
    std::string     m_UserDefinedName;
    std::string     m_FriendlyName;
    std::string     m_UDN;
    std::string     m_BaseURL;
    std::string     m_RelURL;
    std::string     m_PresURL;
    std::string     m_Location;
    std::string     m_ContainerId;
    
    std::chrono::system_clock::time_point   m_TimeoutTime;
    
    std::map<ServiceType, Service>    m_Services;
    
    static const std::string deviceTypeToString(DeviceType type)
    {
        switch (type)
        {
            case DeviceType::MediaServer:
                return MediaServerDeviceTypeUrn;
            case DeviceType::MediaRenderer:
                return MediaRendererDeviceTypeUrn;
            default:
                throw std::logic_error("Invalid device type encountered");
        }
    }
    
    static const DeviceType stringToDeviceType(const std::string& type)
    {
        if (type == MediaServerDeviceTypeUrn)   { return DeviceType::MediaServer; }
        if (type == MediaRendererDeviceTypeUrn) { return DeviceType::MediaRenderer; }
        
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

    virtual void onUPnPDeviceEvent(const Device& item, DeviceEvent event) = 0;
};

}

#endif
