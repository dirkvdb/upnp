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

#include "upnp/upnp.servicefaults.h"

namespace upnp
{
namespace ConnectionManager
{

static const int32_t UnknownConnectionId = -1;
static const int32_t DefaultConnectionId = 0;

DEFINE_UPNP_SERVICE_FAULT(IncompatibleProtocol,                     701, "Incompatible protocol info")
DEFINE_UPNP_SERVICE_FAULT(IncompatibleDirections,                   702, "Incompatible directions")
DEFINE_UPNP_SERVICE_FAULT(InsufficientNetworkResources,             703, "Insufficient network resources")
DEFINE_UPNP_SERVICE_FAULT(LocalRestrictions,                        704, "Local restrictions")
DEFINE_UPNP_SERVICE_FAULT(AccessDenied,                             705, "Access denied")
DEFINE_UPNP_SERVICE_FAULT(InvalidConnectionReference,               706, "Invalid connection reference")
DEFINE_UPNP_SERVICE_FAULT(NotInNetwork,                             707, "Not in network")
DEFINE_UPNP_SERVICE_FAULT(ConnectionTableOverflow,                  708, "Connection table overflow")
DEFINE_UPNP_SERVICE_FAULT(InternalProcessingResourcesExceeded,      709, "Internal processing resources exceeded")
DEFINE_UPNP_SERVICE_FAULT(InternalMemoryResourcesExceeded,          710, "Internal memory resources exceeded")
DEFINE_UPNP_SERVICE_FAULT(InternalStoragSystemCapabilitiesExceeded, 711, "Internal storage system capabilities exceeded")

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
