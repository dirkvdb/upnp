

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

#include "upnp/upnp.types.h"
#include "upnp.enumutils.h"

namespace upnp
{

#define ContentDirectoryServiceTypeUrnBase  "urn:schemas-upnp-org:service:ContentDirectory:"
#define RenderingControlServiceTypeUrnBase  "urn:schemas-upnp-org:service:RenderingControl:"
#define ConnectionManagerServiceTypeUrnBase "urn:schemas-upnp-org:service:ConnectionManager:"
#define AVTransportServiceTypeUrnBase       "urn:schemas-upnp-org:service:AVTransport:"

#define MediaServerDeviceTypeUrnBase        "urn:schemas-upnp-org:device:MediaServer:"
#define MediaRendererDeviceTypeUrnBase      "urn:schemas-upnp-org:device:MediaRenderer:"
#define InternetGatewayDeviceTypeUrnBase    "urn:schemas-upnp-org:device:InternetGatewayDevice:"

static std::array<const char*, 3> ContentDirectoryServiceTypeUrn {{
    ContentDirectoryServiceTypeUrnBase "1",
    ContentDirectoryServiceTypeUrnBase "2",
    ContentDirectoryServiceTypeUrnBase "3"
}};

static std::array<const char*, 3> RenderingControlServiceTypeUrn {{
    RenderingControlServiceTypeUrnBase "1",
    RenderingControlServiceTypeUrnBase "2",
    RenderingControlServiceTypeUrnBase "3"
}};

static std::array<const char*, 3> ConnectionManagerServiceTypeUrn {{
    ConnectionManagerServiceTypeUrnBase "1",
    ConnectionManagerServiceTypeUrnBase "2",
    ConnectionManagerServiceTypeUrnBase "3"
}};

static std::array<const char*, 3> AVTransportServiceTypeUrn {{
    AVTransportServiceTypeUrnBase "1",
    AVTransportServiceTypeUrnBase "2",
    AVTransportServiceTypeUrnBase "3"
}};

static std::array<const char*, 3> MediaServerDeviceTypeUrn {{
    MediaServerDeviceTypeUrnBase "1",
    MediaServerDeviceTypeUrnBase "2",
    MediaServerDeviceTypeUrnBase "3"
}};

static std::array<const char*, 3> MediaRendererDeviceTypeUrn {{
    MediaRendererDeviceTypeUrnBase "1",
    MediaRendererDeviceTypeUrnBase "2",
    MediaRendererDeviceTypeUrnBase "3"
}};

static std::array<const char*, 3> InternetGatewayDeviceTypeUrn {{
    InternetGatewayDeviceTypeUrnBase "1",
    InternetGatewayDeviceTypeUrnBase "2",
    InternetGatewayDeviceTypeUrnBase "3"
}};

const char* RenderingControlServiceIdUrn        = "urn:upnp-org:serviceId:RenderingControl";
const char* ConnectionManagerServiceIdUrn       = "urn:upnp-org:serviceId:ConnectionManager";
const char* AVTransportServiceIdUrn             = "urn:upnp-org:serviceId:AVTransport";
const char* ContentDirectoryServiceIdUrn        = "urn:upnp-org:serviceId:ContentDirectory";

const char* RenderingControlServiceMetadataUrn  = "urn:schemas-upnp-org:metadata-1-0/RCS/";
const char* ConnectionManagerServiceMetadataUrn = "urn:schemas-upnp-org:metadata-1-0/CMS/";
const char* AVTransportServiceMetadataUrn       = "urn:schemas-upnp-org:metadata-1-0/AVT/";

static constexpr EnumMap<Property> s_propertyNames {{
    { "id",                         Property::Id },
    { "parentID",                   Property::ParentId },
    { "dc:title",                   Property::Title },
    { "dc:creator",                 Property::Creator },
    { "dc:date",                    Property::Date },
    { "dc:description",             Property::Description },
    { "res",                        Property::Res },
    { "upnp:class",                 Property::Class },
    { "restricted",                 Property::Restricted },
    { "writeStatus",                Property::WriteStatus },
    { "@refID",                     Property::RefId },
    { "childCount",                 Property::ChildCount },
    { "upnp:createClass",           Property::CreateClass },
    { "upnp:searchClass",           Property::SearchClass },
    { "searchable",                 Property::Searchable },
    { "upnp:artist",                Property::Artist },
    { "upnp:album",                 Property::Album },
    { "upnp:albumArtist",           Property::AlbumArtist },
    { "upnp:albumArtURI",           Property::AlbumArt },
    { "upnp:icon",                  Property::Icon },
    { "upnp:genre",                 Property::Genre },
    { "upnp:originalTrackNumber",   Property::TrackNumber },
    { "upnp:actor",                 Property::Actor },
    { "upnp:storageUsed",           Property::StorageUsed },
    { "*",                          Property::All },
}};

static constexpr EnumMap<ServiceType::Type> s_serviceTypeNames {{
    { "ContentDirectory",     ServiceType::ContentDirectory },
    { "RenderingControl",     ServiceType::RenderingControl },
    { "ConnectionManager",    ServiceType::ConnectionManager },
    { "AVTransport",          ServiceType::AVTransport }
}};

static constexpr EnumMap<Class> s_classNames {{
    { "object.container",                   Class::Container },
    { "object.container.videoContainer",    Class::VideoContainer },
    { "object.container.album.musicAlbum",  Class::AudioContainer },
    { "object.container.photoAlbum",        Class::ImageContainer },
    { "object.container.storageFolder",     Class::StorageFolder },
    { "object.item.videoItem",              Class::Video },
    { "object.item.audioItem",              Class::Audio },
    { "object.item.imageItem",              Class::Image },
    { "object.generic",                     Class::Generic }
}};

static constexpr EnumMap<ErrorCode> s_errorNames {{
    { "Success",                ErrorCode::Success },
    { "Unexpected",             ErrorCode::Unexpected },
    { "Invalid argument",       ErrorCode::InvalidArgument },
    { "Bad request",            ErrorCode::BadRequest },
    { "Precondition failed",    ErrorCode::PreconditionFailed },
    { "Network error",          ErrorCode::NetworkError },
    { "Http error",             ErrorCode::HttpError },
}};

static constexpr EnumMap<ErrorCode, int32_t> s_errorValues {{
    { 0,        ErrorCode::Success },
    { 1,        ErrorCode::Unexpected },
    { 2,        ErrorCode::InvalidArgument },
    { 400,      ErrorCode::BadRequest },
    { 412,      ErrorCode::PreconditionFailed },
    { 3,        ErrorCode::NetworkError },
    { 4,        ErrorCode::HttpError },
}};

ADD_ENUM_MAP(Property, s_propertyNames)
ADD_ENUM_MAP(ServiceType::Type, s_serviceTypeNames)
ADD_ENUM_MAP(Class, s_classNames)
ADD_ENUM_MAP(ErrorCode, s_errorNames)
ADD_ENUM_MAP_TYPED(ErrorCode, int32_t, s_errorValues)

const char* errorCodeToString(ErrorCode value)
{
    return enum_string(value);
}

int32_t errorCodeToInt(ErrorCode code)
{
    return enum_typecast<int32_t>(code);
}

Property propertyFromString(const char* data, size_t size)
{
    return enum_cast<Property>(gsl::cstring_span<>(data, size));
}

Property propertyFromString(const std::string& value)
{
    return enum_cast<Property>(value);
}

const char* toString(Property value) noexcept
{
    return enum_string(value);
}

ServiceType::Type serviceTypeFromString(const std::string& value)
{
    return enum_cast<ServiceType::Type>(value);
}

const char* serviceTypeToTypeString(ServiceType value)
{
    return enum_string(value.type);
}

const char* deviceTypeToString(DeviceType type)
{
         if (type.type == DeviceType::MediaServer)      return MediaServerDeviceTypeUrn.at(type.version - 1);
    else if (type.type == DeviceType::MediaRenderer)    return MediaRendererDeviceTypeUrn.at(type.version - 1);
    else if (type.type == DeviceType::InternetGateway)  return InternetGatewayDeviceTypeUrn.at(type.version - 1);
    else throw std::invalid_argument("Invalid device type received for urn");
}

DeviceType stringToDeviceType(const std::string& value)
{
    DeviceType result;

    if (strncmp(MediaServerDeviceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = DeviceType::MediaServer;
    }
    else if (strncmp(MediaRendererDeviceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = DeviceType::MediaRenderer;
    }
    else if (strncmp(InternetGatewayDeviceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = DeviceType::InternetGateway;
    }
    else
    {
        result.type = DeviceType::Unknown;
        return result;
    }

    result.version = value[value.size() - 1] - '0';
    return result;
}

const char* serviceTypeToUrnTypeString(ServiceType type)
{
         if (type.type == ServiceType::RenderingControl)      return RenderingControlServiceTypeUrn.at(type.version - 1);
    else if (type.type == ServiceType::ConnectionManager)     return ConnectionManagerServiceTypeUrn.at(type.version - 1);
    else if (type.type == ServiceType::AVTransport)           return AVTransportServiceTypeUrn.at(type.version - 1);
    else if (type.type == ServiceType::ContentDirectory)      return ContentDirectoryServiceTypeUrn.at(type.version - 1);
    else throw std::invalid_argument("Invalid service type received for urn");
}

const char* serviceTypeToUrnIdString(ServiceType type)
{
    std::string result;

    if (type.type == ServiceType::RenderingControl)      return RenderingControlServiceIdUrn;
    if (type.type == ServiceType::ConnectionManager)     return ConnectionManagerServiceIdUrn;
    if (type.type == ServiceType::AVTransport)           return AVTransportServiceIdUrn;

    throw std::invalid_argument("Invalid service type received for id urn");
}

const char* serviceTypeToUrnMetadataString(ServiceType type)
{
    if (type.type == ServiceType::RenderingControl)      return RenderingControlServiceMetadataUrn;
    if (type.type == ServiceType::ConnectionManager)     return ConnectionManagerServiceMetadataUrn;
    if (type.type == ServiceType::AVTransport)           return AVTransportServiceMetadataUrn;

    throw std::invalid_argument("Invalid service type received for id urn");
}

ServiceType serviceTypeUrnStringToService(const std::string& value)
{
    ServiceType result;

    if (strncmp(ContentDirectoryServiceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = ServiceType::ContentDirectory;
    }
    else if (strncmp(RenderingControlServiceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = ServiceType::RenderingControl;
    }
    else if (strncmp(ConnectionManagerServiceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = ServiceType::ConnectionManager;
    }
    else if (strncmp(AVTransportServiceTypeUrnBase, value.data(), value.size() - 1) == 0)
    {
        result.type = ServiceType::AVTransport;
    }
    else
    {
        result.type = ServiceType::Unknown;
        return result;
    }

    result.version = value[value.size() - 1] - '0';
    return result;
}

ServiceType::Type serviceIdUrnStringToService(const std::string& type)
{
    if (type == ContentDirectoryServiceIdUrn)    return ServiceType::ContentDirectory;
    if (type == RenderingControlServiceIdUrn)    return ServiceType::RenderingControl;
    if (type == ConnectionManagerServiceIdUrn)   return ServiceType::ConnectionManager;
    if (type == AVTransportServiceIdUrn)         return ServiceType::AVTransport;

    return ServiceType::Unknown;
}

const char* toString(Class value) noexcept
{
    return enum_string(value);
}

}
