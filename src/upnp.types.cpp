

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

static const char* RenderingControlServiceIdUrn        = "urn:upnp-org:serviceId:RenderingControl";
static const char* ConnectionManagerServiceIdUrn       = "urn:upnp-org:serviceId:ConnectionManager";
static const char* AVTransportServiceIdUrn             = "urn:upnp-org:serviceId:AVTransport";
static const char* ContentDirectoryServiceIdUrn        = "urn:upnp-org:serviceId:ContentDirectory";

static const char* RenderingControlServiceMetadataUrn  = "urn:schemas-upnp-org:metadata-1-0/RCS/";
static const char* ConnectionManagerServiceMetadataUrn = "urn:schemas-upnp-org:metadata-1-0/CMS/";
static const char* AVTransportServiceMetadataUrn       = "urn:schemas-upnp-org:metadata-1-0/AVT/";

static constexpr EnumMap<Property> s_propertyNames {{
    std::make_tuple("id",                         Property::Id),
    std::make_tuple("parentID",                   Property::ParentId),
    std::make_tuple("dc:title",                   Property::Title),
    std::make_tuple("dc:creator",                 Property::Creator),
    std::make_tuple("dc:date",                    Property::Date),
    std::make_tuple("dc:description",             Property::Description),
    std::make_tuple("res",                        Property::Res),
    std::make_tuple("upnp:class",                 Property::Class),
    std::make_tuple("restricted",                 Property::Restricted),
    std::make_tuple("writeStatus",                Property::WriteStatus),
    std::make_tuple("@refID",                     Property::RefId),
    std::make_tuple("childCount",                 Property::ChildCount),
    std::make_tuple("upnp:createClass",           Property::CreateClass),
    std::make_tuple("upnp:searchClass",           Property::SearchClass),
    std::make_tuple("searchable",                 Property::Searchable),
    std::make_tuple("upnp:artist",                Property::Artist),
    std::make_tuple("upnp:album",                 Property::Album),
    std::make_tuple("upnp:albumArtist",           Property::AlbumArtist),
    std::make_tuple("upnp:albumArtURI",           Property::AlbumArt),
    std::make_tuple("upnp:icon",                  Property::Icon),
    std::make_tuple("upnp:genre",                 Property::Genre),
    std::make_tuple("upnp:originalTrackNumber",   Property::TrackNumber),
    std::make_tuple("upnp:actor",                 Property::Actor),
    std::make_tuple("upnp:storageUsed",           Property::StorageUsed),
    std::make_tuple("*",                          Property::All),
}};

static constexpr EnumMap<ServiceType::Type> s_serviceTypeNames {{
    std::make_tuple("ContentDirectory",     ServiceType::ContentDirectory),
    std::make_tuple("RenderingControl",     ServiceType::RenderingControl),
    std::make_tuple("ConnectionManager",    ServiceType::ConnectionManager),
    std::make_tuple("AVTransport",          ServiceType::AVTransport)
}};

static constexpr EnumMap<Class> s_classNames {{
    std::make_tuple("object.container",                   Class::Container),
    std::make_tuple("object.container.videoContainer",    Class::VideoContainer),
    std::make_tuple("object.container.album.musicAlbum",  Class::AudioContainer),
    std::make_tuple("object.container.photoAlbum",        Class::ImageContainer),
    std::make_tuple("object.container.storageFolder",     Class::StorageFolder),
    std::make_tuple("object.item.videoItem",              Class::Video),
    std::make_tuple("object.item.audioItem",              Class::Audio),
    std::make_tuple("object.item.imageItem",              Class::Image),
    std::make_tuple("object.generic",                     Class::Generic),
}};

static constexpr EnumMap<ErrorCode> s_errorNames {{
    std::make_tuple("Success",                ErrorCode::Success),
    std::make_tuple("Unexpected",             ErrorCode::Unexpected),
    std::make_tuple("Invalid argument",       ErrorCode::InvalidArgument),
    std::make_tuple("Bad request",            ErrorCode::BadRequest),
    std::make_tuple("Precondition failed",    ErrorCode::PreconditionFailed),
    std::make_tuple("Network error",          ErrorCode::NetworkError),
    std::make_tuple("Http error",             ErrorCode::HttpError),
}};

static constexpr EnumMap<ErrorCode, int32_t> s_errorValues {{
    std::make_tuple(0,        ErrorCode::Success),
    std::make_tuple(1,        ErrorCode::Unexpected),
    std::make_tuple(2,        ErrorCode::InvalidArgument),
    std::make_tuple(400,      ErrorCode::BadRequest),
    std::make_tuple(412,      ErrorCode::PreconditionFailed),
    std::make_tuple(3,        ErrorCode::NetworkError),
    std::make_tuple(4,        ErrorCode::HttpError),
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
    return enum_cast<Property>(std::string_view(data, size));
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

DeviceType deviceTypeFromString(const std::string& value)
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
