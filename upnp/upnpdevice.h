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

#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H

#include <string>

#include "utils/types.h"

namespace UPnP
{

class Device
{
public:
    bool operator==(const Device& otherDevice) const { return m_UDN == otherDevice.m_UDN; }

    std::string     m_UserDefinedName;
    std::string     m_FriendlyName;
    std::string     m_DeviceType;
    std::string     m_UDN;
    std::string     m_BaseURL;
    std::string     m_RelURL;
    std::string     m_PresURL;
    std::string     m_Location;
    std::string     m_CDServiceID;
    std::string     m_CDControlURL;
    std::string     m_CDEventSubURL;
    std::string     m_CDSubscriptionID;
    std::string     m_ContainerId;
    int32_t         m_AdvTimeout;
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
