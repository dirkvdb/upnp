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

#ifndef UPNP_CONNECTION_MANAGER_H
#define UPNP_CONNECTION_MANAGER_H

#include "upnp/upnpservicebase.h"
#include "upnp/upnpprotocolinfo.h"

namespace upnp
{

class Action;

enum class ConnectionManagerAction
{
    GetProtocolInfo,
    PrepareForConnection, // Optional
    ConnectionComplete, // Optional
    GetCurrentConnectionIDs,
    GetCurrentConnectionInfo
};

enum class ConnectionManagerVariable
{
    SourceProtocolInfo,
    SinkProtocolInfo,
    CurrentConnectionIds,
    ArgumentTypeConnectionStatus,
    ArgumentTypeConnectionManager,
    ArgumentTypeDirection,
    ArgumentTypeProtocolInfo,
    ArgumentTypeConnectionId,
    ArgumentTypeAVTransportId,
    ArgumentTypeRecourceId
};
    
class ConnectionManager : public ServiceBase<ConnectionManagerAction, ConnectionManagerVariable>
{
public:    
    static std::string UnknownConnectionId;
    static std::string DefaultConnectionId;

    ConnectionManager(IClient& client);
    
    std::vector<ProtocolInfo> getProtocolInfo();
    ConnectionInfo prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, const std::string& peerConnectionId, Direction direction);
    void connectionComplete(const ConnectionInfo& connectionInfo);
    std::vector<std::string> getCurrentConnectionIds();
    ConnectionInfo getCurrentConnectionInfo(const std::string& connectionId);
    
protected:
    ServiceType getType();
    int32_t getSubscriptionTimeout();
    
    void handleUPnPResult(int errorCode);
    
    ConnectionManagerAction actionFromString(const std::string& action);
    std::string actionToString(ConnectionManagerAction action);
    ConnectionManagerVariable variableFromString(const std::string& var);
    std::string variableToString(ConnectionManagerVariable var);
};
    
}

#endif