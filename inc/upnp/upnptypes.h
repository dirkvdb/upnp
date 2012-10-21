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

#include <stdexcept>

#include "upnp/upnpprotocolinfo.h"

namespace upnp
{

namespace
{
    static const char* ContentDirectoryServiceTypeUrn      = "urn:schemas-upnp-org:service:ContentDirectory:1";
    static const char* RenderingControlServiceTypeUrn      = "urn:schemas-upnp-org:service:RenderingControl:1";
    static const char* ConnectionManagerServiceTypeUrn     = "urn:schemas-upnp-org:service:ConnectionManager:1";
    static const char* AVTransportServiceTypeUrn           = "urn:schemas-upnp-org:service:AVTransport:1";
}

enum class ServiceType
{
    ContentDirectory,
    RenderingControl,
    ConnectionManager,
    AVTransport,
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

enum class Direction
{
    Input,
    Output
};

enum class ConnectionStatus
{
    Ok,
    ContentFormatMismatch,
    InsufficientBandwith,
    UnreliableChannel,
    Unknown
};

class UPnPException : public std::exception
{
public:
    UPnPException(int32_t errorCode) : m_ErrorCode(errorCode) {}
    int32_t getErrorCode() { return m_ErrorCode; }
    
private:
    int32_t     m_ErrorCode;
};

struct ConnectionInfo
{
    std::string         connectionId;
    std::string         avTransportId;
    std::string         renderingControlServiceId;
    ProtocolInfo        protocolInfo;
    std::string         peerConnectionManager;
    std::string         peerConnectionId;
    Direction           direction;
    ConnectionStatus    connectionStatus;
};

inline ServiceType serviceTypeFromString(const std::string& name)
{
    if (name == "ContentDirectory")     return ServiceType::ContentDirectory;
    if (name == "RenderingControl")     return ServiceType::RenderingControl;
    if (name == "ConnectionManager")    return ServiceType::ConnectionManager;
    if (name == "AVTransport")          return ServiceType::AVTransport;
    
    return ServiceType::Unknown;
}

inline std::string serviceTypeToString(ServiceType type)
{
    if (type == ServiceType::ContentDirectory)      return "ContentDirectory";
    if (type == ServiceType::RenderingControl)      return "RenderingControl";
    if (type == ServiceType::ConnectionManager)     return "ConnectionManager";
    if (type == ServiceType::AVTransport)           return "AVTransport";
    if (type == ServiceType::Unknown)               return "UnknownServiceType";
    
    throw std::logic_error("Invalid service type received");
}

inline std::string serviceTypeToUrnString(ServiceType type)
{
    if (type == ServiceType::ContentDirectory)      return ContentDirectoryServiceTypeUrn;
    if (type == ServiceType::RenderingControl)      return RenderingControlServiceTypeUrn;
    if (type == ServiceType::ConnectionManager)     return ConnectionManagerServiceTypeUrn;
    if (type == ServiceType::AVTransport)           return AVTransportServiceTypeUrn;
    
    throw std::logic_error("Invalid service type received for urn");
}

inline ServiceType stringToServiceTypeUrn(const std::string& type)
{
    if (type == ContentDirectoryServiceTypeUrn)    return ServiceType::ContentDirectory;
    if (type == RenderingControlServiceTypeUrn)    return ServiceType::RenderingControl;
    if (type == ConnectionManagerServiceTypeUrn)   return ServiceType::ConnectionManager;
    if (type == AVTransportServiceTypeUrn)         return ServiceType::AVTransport;
    
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
    if (name == "upnp:albumArtURI")         return Property::AlbumArt;
    if (name == "upnp:icon")                return Property::Icon;
    if (name == "upnp:genre")               return Property::Genre;
    if (name == "upnp:originalTrackNumber") return Property::TrackNumber;
    if (name == "upnp:actor")               return Property::Actor;
    if (name == "upnp:storageUsed")         return Property::StorageUsed;
    if (name == "*")                        return Property::All;
    
    return Property::Unknown;
}

inline std::string propertyToString(Property prop)
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

inline std::string directionToString(Direction direction)
{
    switch (direction)
    {
        case Direction::Input:      return "Input";
        case Direction::Output:     return "Output";
        default:                    throw std::logic_error("Invalid direction specified");
    }
}

inline Direction directionFromString(const std::string& direction)
{
    if (direction == "Input")   return Direction::Input;
    if (direction == "Output")  return Direction::Output;
    
    throw std::logic_error("Invalid direction received");
}

inline ConnectionStatus connectionStatusFromString(const std::string& status)
{
    if (status == "OK")                     return ConnectionStatus::Ok;
    if (status == "ContentFormatMismatch")  return ConnectionStatus::ContentFormatMismatch;
    if (status == "InsufficientBandwith")   return ConnectionStatus::InsufficientBandwith;
    if (status == "UnreliableChannel")      return ConnectionStatus::UnreliableChannel;
    
    return ConnectionStatus::Unknown;
}

}

#endif