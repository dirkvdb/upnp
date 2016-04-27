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

#include "upnp/upnpdeviceserviceexceptions.h"

namespace upnp
{
namespace ConnectionManager
{

static const int32_t UnknownConnectionId = -1;
static const int32_t DefaultConnectionId = 0;

DEFINE_UPNP_SERVICE_EXCEPTION(IncompatibleProtocol,                     "Incompatible protocol info",                       701)
DEFINE_UPNP_SERVICE_EXCEPTION(IncompatibleDirections,                   "Incompatible directions",                          702)
DEFINE_UPNP_SERVICE_EXCEPTION(InsufficientNetworkResources,             "Insufficient network resources",                   703)
DEFINE_UPNP_SERVICE_EXCEPTION(LocalRestrictions,                        "Local restrictions",                               704)
DEFINE_UPNP_SERVICE_EXCEPTION(AccessDenied,                             "Access denied",                                    705)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidConnectionReference,               "Invalid connection reference",                     706)
DEFINE_UPNP_SERVICE_EXCEPTION(NotInNetwork,                             "Not in network",                                   707)
DEFINE_UPNP_SERVICE_EXCEPTION(ConnectionTableOverflow,                  "Connection table overflow",                        708)
DEFINE_UPNP_SERVICE_EXCEPTION(InternalProcessingResourcesExceeded,      "Internal processing resources exceeded",           709)
DEFINE_UPNP_SERVICE_EXCEPTION(InternalMemoryResourcesExceeded,          "Internal memory resources exceeded",               710)
DEFINE_UPNP_SERVICE_EXCEPTION(InternalStoragSystemCapabilitiesExceeded, "Internal storage system capabilities exceeded",    711)

enum class Action
{
// ConnectionManager:1
    GetProtocolInfo,
    PrepareForConnection, // Optional
    ConnectionComplete, // Optional
    GetCurrentConnectionIDs,
    GetCurrentConnectionInfo,

// ConnectionManager:3
    GetRendererItemInfo, // Optional
    GetFeatureList,

    EnumCount
};

enum class Variable
{
// ConnectionManager:1
    SourceProtocolInfo,
    SinkProtocolInfo,
    CurrentConnectionIds,
    ArgumentTypeConnectionStatus,
    ArgumentTypeConnectionManager,
    ArgumentTypeDirection,
    ArgumentTypeProtocolInfo,
    ArgumentTypeConnectionId,
    ArgumentTypeAVTransportId,
    ArgumentTypeRecourceId,

// ConnectionManager:3
    ArgumentTypeItemInfoFilter,
    ArgumentTypeResult,
    ArgumentTypeRenderingInfoList,

    EnumCount
};

enum class ConnectionStatus
{
    Ok,
    ContentFormatMismatch,
    InsufficientBandwith,
    UnreliableChannel,
    Unknown
};

enum class Direction
{
    Input,
    Output,

    EnumCount
};

struct ConnectionInfo
{
    int32_t             connectionId = UnknownConnectionId;
    int32_t             avTransportId = 0;
    int32_t             renderingControlServiceId = 0;
    ProtocolInfo        protocolInfo;
    std::string         peerConnectionManager;
    int32_t             peerConnectionId = 0;
    Direction           direction;
    ConnectionStatus    connectionStatus = ConnectionStatus::Unknown;
};

}
}
