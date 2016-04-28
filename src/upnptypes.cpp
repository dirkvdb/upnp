

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
#include "upnp.enumutils.h"

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

constexpr std::tuple<const char*, Property> s_propertyNames[] {
    { "id", Property::Id },
    { "parentID", Property::ParentId },
    { "dc:title", Property::Title },
    { "dc:creator", Property::Creator },
    { "dc:date", Property::Date },
    { "dc:description", Property::Description },
    { "res", Property::Res },
    { "upnp:class", Property::Class },
    { "restricted", Property::Restricted },
    { "writeStatus", Property::WriteStatus },
    { "@refID", Property::RefId },
    { "childCount", Property::ChildCount },
    { "upnp:createClass", Property::CreateClass },
    { "upnp:searchClass", Property::SearchClass },
    { "searchable", Property::Searchable },
    { "upnp:artist", Property::Artist },
    { "upnp:album", Property::Album },
    { "upnp:albumArtist", Property::AlbumArtist },
    { "upnp:albumArtURI", Property::AlbumArt },
    { "upnp:icon", Property::Icon },
    { "upnp:genre", Property::Genre },
    { "upnp:originalTrackNumber", Property::TrackNumber },
    { "upnp:actor", Property::Actor },
    { "upnp:storageUsed", Property::StorageUsed },
    { "*", Property::All },
};

constexpr std::tuple<const char*, ServiceType> s_serviceTypeNames[] {
    { "ContentDirectory",     ServiceType::ContentDirectory },
    { "RenderingControl",     ServiceType::RenderingControl },
    { "ConnectionManager",    ServiceType::ConnectionManager },
    { "AVTransport",          ServiceType::AVTransport }
};

constexpr std::tuple<const char*, Class> s_classNames[] {
    { "object.container",                   Class::Container },
    { "object.container.videoContainer",    Class::VideoContainer },
    { "object.container.album.musicAlbum",  Class::AudioContainer },
    { "object.container.photoAlbum",        Class::ImageContainer },
    { "object.container.storageFolder",     Class::StorageFolder },
    { "object.item.videoItem",              Class::Video },
    { "object.item.audioItem",              Class::Audio },
    { "object.item.imageItem",              Class::Image },
    { "object.generic",                     Class::Generic }
};

ADD_ENUM_MAP(Property, s_propertyNames)
ADD_ENUM_MAP(ServiceType, s_serviceTypeNames)
ADD_ENUM_MAP(Class, s_classNames)

Property propertyFromString(const char* data, size_t dataSize)
{
    return enum_cast<Property>(data, dataSize);
}

Property propertyFromString(const std::string& value)
{
    return enum_cast<Property>(value.data(), value.size());
}

const char* toString(Property value) noexcept
{
    return string_cast(value);
}

ServiceType serviceTypeFromString(const std::string& value)
{
    return enum_cast<ServiceType>(value.data(), value.size());
}

const char* serviceTypeToTypeString(ServiceType type)
{
    return string_cast(type);
}

const char* toString(Class value) noexcept
{
    return string_cast(value);
}

}