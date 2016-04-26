

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

#include "upnp/upnptypes.h"

#include <array>

namespace upnp
{

const char* ContentDirectoryServiceTypeUrn      = "urn:schemas-upnp-org:service:ContentDirectory:1";
const char* RenderingControlServiceTypeUrn      = "urn:schemas-upnp-org:service:RenderingControl:1";
const char* ConnectionManagerServiceTypeUrn     = "urn:schemas-upnp-org:service:ConnectionManager:1";
const char* AVTransportServiceTypeUrn           = "urn:schemas-upnp-org:service:AVTransport:1";

const char* RenderingControlServiceIdUrn        = "urn:upnp-org:serviceId:RenderingControl";
const char* ConnectionManagerServiceIdUrn       = "urn:upnp-org:serviceId:ConnectionManager";
const char* AVTransportServiceIdUrn             = "urn:upnp-org:serviceId:AVTransport";
const char* ContentDirectoryServiceIdUrn        = "urn:upnp-org:serviceId:ContentDirectory";

const char* RenderingControlServiceMetadataUrn  = "urn:schemas-upnp-org:metadata-1-0/RCS/";
const char* ConnectionManagerServiceMetadataUrn = "urn:schemas-upnp-org:metadata-1-0/CMS/";
const char* AVTransportServiceMetadataUrn       = "urn:schemas-upnp-org:metadata-1-0/AVT/";

static constexpr std::array<const char*, static_cast<uint32_t>(Property::Unknown)> s_propertyNames {{
    "id",
    "parentID",
    "dc:title",
    "dc:creator",
    "dc:date",
    "dc:description",
    "res",
    "upnp:class",
    "restricted",
    "writeStatus",
    "@refID",
    "childCount",
    "upnp:createClass",
    "upnp:searchClass",
    "searchable",
    "upnp:artist",
    "upnp:album",
    "upnp:albumArtist",
    "upnp:albumArtURI",
    "upnp:icon",
    "upnp:genre",
    "upnp:originalTrackNumber",
    "upnp:actor",
    "upnp:storageUsed",
    "*"
}};

Property propertyFromString(const char* data, size_t dataSize)
{
    for (uint32_t i = 0; i < s_propertyNames.size(); ++i)
    {
        if (strncmp(s_propertyNames[i], data, dataSize) == 0)
        {
            return static_cast<Property>(i);
        }
    }

    return Property::Unknown;
}

Property propertyFromString(const std::string& value)
{
    return propertyFromString(value.c_str(), value.size());
}

const char* toString(Property value) noexcept
{
    assert(static_cast<uint32_t>(value) < s_propertyNames.size());
    return s_propertyNames[static_cast<int>(value)];
}

}
