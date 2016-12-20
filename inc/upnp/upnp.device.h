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

#pragma once

#include <string>
#include <chrono>
#include <cinttypes>
#include <map>

#include "upnp/upnp.types.h"

namespace upnp
{

struct Service
{
    ServiceType     type;
    std::string     id;
    std::string     scpdUrl;
    std::string     controlURL;
    std::string     eventSubscriptionURL;
    std::string     eventSubscriptionID;
};

struct Device
{
    bool operator==(const Device& otherDevice) const { return udn == otherDevice.udn; }
    bool implementsService(ServiceType type) const { return services.find(type.type) != services.end(); }

    DeviceType      type;
    uint8_t         majorVersion = 0;
    uint8_t         minorVersion = 0;
    std::string     userDefinedName;
    std::string     friendlyName;
    std::string     udn;
    std::string     baseURL;
    std::string     relURL;
    std::string     presURL;
    std::string     location;
    std::string     containerId;

    std::chrono::system_clock::time_point timeoutTime;
    std::map<ServiceType::Type, Service> services;
};

}

