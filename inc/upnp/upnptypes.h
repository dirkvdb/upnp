

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

#ifndef UPNP_TYPES_H
#define UPNP_TYPES_H

#include <map>
#include <string>
#include <memory>
#include <stdexcept>

#include "utils/format.h"
#include "upnp/upnpfwd.h"
#include "upnp/upnpprotocolinfo.h"

namespace upnp
{

namespace
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
}

enum class ServiceType
{
    ContentDirectory,
    RenderingControl,
    ConnectionManager,
    AVTransport,
    Unknown
};

enum class DeviceType
{
    MediaServer,
    MediaRenderer,
    InternetGateway,
    Unknown
};

enum class Property
{
    Id,
    ParentId,
    Title,
    Creator,
    Date,
    Res,
    Class,
    Restricted,
    WriteStatus,
    RefId,
    ChildCount,
    CreateClass,
    SearchClass,
    Searchable,
    Artist,
    AlbumArtist,
    Album,
    AlbumArt,
    Icon,
    Genre,
    TrackNumber,
    Actor,
    All,
    Description,
    StorageUsed,
    Unknown
};

enum class Class
{
    Container,
    VideoContainer,
    AudioContainer,
    ImageContainer,
    StorageFolder,
    Video,
    Audio,
    Image,
    Generic,
    Unknown
};

enum class ErrorCode : int32_t
{
    BadRequest = 400,
    PreconditionFailed = 412
};

inline std::string errorCodeToString(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::BadRequest:             return "Bad request";
    case ErrorCode::PreconditionFailed:     return "Precondition Failed";
    default:                                return "Unspecified error";
    }
}

class Exception : public std::runtime_error
{
public:
    Exception(const char* msg) : std::runtime_error(msg), m_errorCode(0) {}
    Exception(const std::string& msg) : Exception(msg.c_str()) {}

    template<typename... T>
    Exception(const char* fmt, const T&... args) : Exception(0, fmt, std::forward<const T>(args)...) {}

    Exception(int32_t errorCode, const char* msg) : std::runtime_error(msg), m_errorCode(errorCode) {}
    Exception(int32_t errorCode, const std::string& msg) : Exception(errorCode, msg.c_str()) {}

    Exception(ErrorCode errorCode) : Exception(static_cast<int32_t>(errorCode), errorCodeToString(errorCode)) {}

    template<typename... T>
    Exception(int32_t errorCode, const char* fmt, T&&... args) : Exception(errorCode, fmt::format(fmt, std::forward<T&&>(args)...)) {}

    int32_t getErrorCode() { return m_errorCode; }

private:
    int32_t     m_errorCode;
};

inline ServiceType serviceTypeFromString(const std::string& name)
{
    if (name == "ContentDirectory")     return ServiceType::ContentDirectory;
    if (name == "RenderingControl")     return ServiceType::RenderingControl;
    if (name == "ConnectionManager")    return ServiceType::ConnectionManager;
    if (name == "AVTransport")          return ServiceType::AVTransport;

    return ServiceType::Unknown;
}

inline std::string serviceTypeToTypeString(ServiceType type)
{
    if (type == ServiceType::ContentDirectory)      return "ContentDirectory";
    if (type == ServiceType::RenderingControl)      return "RenderingControl";
    if (type == ServiceType::ConnectionManager)     return "ConnectionManager";
    if (type == ServiceType::AVTransport)           return "AVTransport";
    if (type == ServiceType::Unknown)               return "UnknownServiceType";

    throw Exception("Invalid service type received");
}

inline const char* serviceTypeToUrnTypeString(ServiceType type)
{
    if (type == ServiceType::ContentDirectory)      return ContentDirectoryServiceTypeUrn;
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceTypeUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceTypeUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceTypeUrn;

    throw Exception("Invalid service type received for urn");
}

inline std::string serviceTypeToUrnIdString(ServiceType type)
{
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceIdUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceIdUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceIdUrn;

    throw Exception("Invalid service type received for id urn");
}

inline std::string serviceTypeToUrnMetadataString(ServiceType type)
{
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceMetadataUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceMetadataUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceMetadataUrn;

    throw Exception("Invalid service type received for id urn");
}

inline ServiceType serviceTypeUrnStringToService(const std::string& type)
{
    if (type == ContentDirectoryServiceTypeUrn)    return ServiceType::ContentDirectory;
    if (type == RenderingControlServiceTypeUrn)    return ServiceType::RenderingControl;
    if (type == ConnectionManagerServiceTypeUrn)   return ServiceType::ConnectionManager;
    if (type == AVTransportServiceTypeUrn)         return ServiceType::AVTransport;

    return ServiceType::Unknown;
}

inline ServiceType serviceIdUrnStringToService(const std::string& type)
{
    if (type == ContentDirectoryServiceIdUrn)    return ServiceType::ContentDirectory;
    if (type == RenderingControlServiceIdUrn)    return ServiceType::RenderingControl;
    if (type == ConnectionManagerServiceIdUrn)   return ServiceType::ConnectionManager;
    if (type == AVTransportServiceIdUrn)         return ServiceType::AVTransport;

    return ServiceType::Unknown;
}

inline Property propertyFromString(const std::string& name)
{
    if (name == "id")                       return Property::Id;
    if (name == "parentID")                 return Property::ParentId;
    if (name == "dc:title")                 return Property::Title;
    if (name == "dc:creator")               return Property::Creator;
    if (name == "dc:date")                  return Property::Date;
    if (name == "dc:description")           return Property::Description;
    if (name == "res")                      return Property::Res;
    if (name == "upnp:class")               return Property::Class;
    if (name == "restricted")               return Property::Restricted;
    if (name == "writeStatus")              return Property::WriteStatus;
    if (name == "@refID")                   return Property::RefId;
    if (name == "childCount")               return Property::ChildCount;
    if (name == "upnp:createClass")         return Property::CreateClass;
    if (name == "upnp:searchClass")         return Property::SearchClass;
    if (name == "searchable")               return Property::Searchable;
    if (name == "upnp:artist")              return Property::Artist;
    if (name == "upnp:album")               return Property::Album;
    if (name == "upnp:albumArtist")         return Property::AlbumArtist;
    if (name == "upnp:albumArtURI")         return Property::AlbumArt;
    if (name == "upnp:icon")                return Property::Icon;
    if (name == "upnp:genre")               return Property::Genre;
    if (name == "upnp:originalTrackNumber") return Property::TrackNumber;
    if (name == "upnp:actor")               return Property::Actor;
    if (name == "upnp:storageUsed")         return Property::StorageUsed;
    if (name == "*")                        return Property::All;

    return Property::Unknown;
}

inline std::string toString(Property prop)
{
    switch (prop)
    {
    case Property::Id:                return "id";
    case Property::ParentId:          return "parentID";
    case Property::Title:             return "dc:title";
    case Property::Creator:           return "dc:creator";
    case Property::Date:              return "dc:date";
    case Property::Description:       return "dc:description";
    case Property::Res:               return "res";
    case Property::Class:             return "upnp:class";
    case Property::Restricted:        return "restricted";
    case Property::WriteStatus:       return "writeStatus";
    case Property::RefId:             return "@refID";
    case Property::ChildCount:        return "childCount";
    case Property::CreateClass:       return "upnp:createClass";
    case Property::SearchClass:       return "upnp:searchClass";
    case Property::Searchable:        return "searchable";
    case Property::Artist:            return "upnp:artist";
    case Property::Album:             return "upnp:album";
    case Property::AlbumArtist:       return "upnp:albumArtist";
    case Property::AlbumArt:          return "upnp:albumArtURI";
    case Property::Icon:              return "upnp:icon";
    case Property::Genre:             return "upnp:genre";
    case Property::TrackNumber:       return "upnp:originalTrackNumber";
    case Property::Actor:             return "upnp:actor";
    case Property::StorageUsed:       return "upnp:storageUsed";
    case Property::All:               return "*";
    case Property::Unknown:
    default:                          return "";
    }
}

inline std::string toString(Class c)
{
    switch (c)
    {
    case Class::Container:          return "object.container";
    case Class::VideoContainer:     return "object.container.videoContainer";
    case Class::AudioContainer:     return "object.container.album.musicAlbum";
    case Class::ImageContainer:     return "object.container.photoAlbum";
    case Class::StorageFolder:      return "object.container.storageFolder";
    case Class::Video:              return "object.item.videoItem";
    case Class::Audio:              return "object.item.audioItem";
    case Class::Image:              return "object.item.imageItem";
    case Class::Generic:            return "object.generic";
    default:
        throw Exception("Invalid upnp class provided");
    }
}

}

#endif

