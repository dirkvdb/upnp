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

#include "upnp/upnp.device.h"
#include "upnp.enumutils.h"

namespace upnp
{

static constexpr const char* MediaServerDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaServer:1";
static constexpr const char* MediaRendererDeviceTypeUrn = "urn:schemas-upnp-org:device:MediaRenderer:1";
static constexpr const char* InternetGatewayDeviceTypeUrn = "urn:schemas-upnp-org:device:InternetGatewayDevice:1";

static constexpr EnumMap<DeviceType> s_deviceTypeNames {{
    { MediaServerDeviceTypeUrn,          DeviceType::MediaServer },
    { MediaRendererDeviceTypeUrn,        DeviceType::MediaRenderer },
    { InternetGatewayDeviceTypeUrn,      DeviceType::InternetGateway },
}};

ADD_ENUM_MAP(DeviceType, s_deviceTypeNames)

const char* Device::deviceTypeToString(DeviceType value)
{
    return enum_string(value);
}

DeviceType Device::stringToDeviceType(const std::string& value)
{
    return enum_cast<DeviceType>(value);
}

}

