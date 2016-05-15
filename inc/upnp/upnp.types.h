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

#pragma once

#include <string>
#include <cinttypes>

namespace upnp
{

extern const char* ContentDirectoryServiceTypeUrn;
extern const char* RenderingControlServiceTypeUrn;
extern const char* ConnectionManagerServiceTypeUrn;
extern const char* AVTransportServiceTypeUrn;

extern const char* RenderingControlServiceIdUrn;
extern const char* ConnectionManagerServiceIdUrn;
extern const char* AVTransportServiceIdUrn;
extern const char* ContentDirectoryServiceIdUrn;

extern const char* RenderingControlServiceMetadataUrn;
extern const char* ConnectionManagerServiceMetadataUrn;
extern const char* AVTransportServiceMetadataUrn;

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
    Description,
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
    Album,
    AlbumArtist,
    AlbumArt,
    Icon,
    Genre,
    TrackNumber,
    Actor,
    StorageUsed,
    All,
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

struct SubscriptionEvent
{
    std::string sid;
    std::string data;
    uint32_t sequence = 0;
};

Property propertyFromString(const char* data, size_t size);
Property propertyFromString(const std::string& name);
const char* toString(Property prop) noexcept;
const char* toString(Class c) noexcept;

ServiceType serviceTypeFromString(const std::string& name);
const char* serviceTypeToTypeString(ServiceType type);

inline const char* serviceTypeToUrnTypeString(ServiceType type)
{
    if (type == ServiceType::ContentDirectory)      return ContentDirectoryServiceTypeUrn;
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceTypeUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceTypeUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceTypeUrn;

    throw std::invalid_argument("Invalid service type received for urn");
}

inline const char* serviceTypeToUrnIdString(ServiceType type)
{
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceIdUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceIdUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceIdUrn;

    throw std::invalid_argument("Invalid service type received for id urn");
}

inline const char* serviceTypeToUrnMetadataString(ServiceType type)
{
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceMetadataUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceMetadataUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceMetadataUrn;

    throw std::invalid_argument("Invalid service type received for id urn");
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

}

