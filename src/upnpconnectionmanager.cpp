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

#include "upnp/upnpconnectionmanager.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

using namespace utils;


namespace upnp
{

static const int32_t g_subscriptionTimeout = 1801;

std::string ConnectionManager::UnknownConnectionId = "-1";
std::string ConnectionManager::DefaultConnectionId = "0";

ConnectionManager::ConnectionManager(IClient& client)
: ServiceBase(client)
{
}

std::vector<ProtocolInfo> ConnectionManager::getProtocolInfo()
{
    xml::Document result = executeAction(ConnectionManagerAction::GetProtocolInfo);
    
    std::vector<ProtocolInfo> protocolInfo;
    auto infos = stringops::tokenize(result.getChildNodeValueRecursive("Sink"), ",");
    for (auto& info : infos)
    {
        try
        {
            protocolInfo.push_back(ProtocolInfo(info));
#ifdef DEBUG_CONNECTION_MANAGER
            log::debug(info);
#endif
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
        
        protocolInfo.push_back(ProtocolInfo("http-get:*:audio/m3u:*"));
    }
    
    return protocolInfo;
}

ConnectionInfo ConnectionManager::prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionId, const std::string& peerConnectionManager, Direction direction)
{
    xml::Document result = executeAction(ConnectionManagerAction::PrepareForConnection, { {"RemoteProtocolInfo", protocolInfo.toString()},
                                                                                          {"PeerConnectionManager", peerConnectionManager},
                                                                                          {"PeerConnectionID", peerConnectionId},
                                                                                          {"Direction", directionToString(direction)} });

    ConnectionInfo connInfo;
    connInfo.connectionId               = result.getChildNodeValue("ConnectionID");
    connInfo.avTransportId              = result.getChildNodeValue("AVTransportID");
    connInfo.renderingControlServiceId  = result.getChildNodeValue("RcsID");
    
    return connInfo;
}

void ConnectionManager::connectionComplete(const ConnectionInfo& connectionInfo)
{
    executeAction(ConnectionManagerAction::ConnectionComplete, { {"ConnectionID", connectionInfo.connectionId} });
}

std::vector<std::string> ConnectionManager::getCurrentConnectionIds()
{
    xml::Document result = executeAction(ConnectionManagerAction::GetCurrentConnectionIDs);
    std::vector<std::string> ids = stringops::tokenize(result.getChildNodeValue("ConnectionIDs"), ",");
    
    return ids;
}

ServiceType ConnectionManager::getType()
{
    return ServiceType::ConnectionManager;
}

int32_t ConnectionManager::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

ConnectionInfo ConnectionManager::getCurrentConnectionInfo(const std::string& connectionId)
{
    xml::Document result = executeAction(ConnectionManagerAction::GetCurrentConnectionInfo, { {"ConnectionID", connectionId} });
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = connectionId;
    connInfo.avTransportId              = result.getChildNodeValue("AVTransportID");
    connInfo.renderingControlServiceId  = result.getChildNodeValue("RcsID");
    connInfo.protocolInfo               = ProtocolInfo(result.getChildNodeValue("ProtocolInfo"));
    connInfo.peerConnectionManager      = result.getChildNodeValue("PeerConnectionManager");
    connInfo.peerConnectionId           = result.getChildNodeValue("PeerConnectionID");
    connInfo.direction                  = directionFromString(result.getChildNodeValue("Direction"));
    connInfo.connectionStatus           = connectionStatusFromString(result.getChildNodeValue("Status"));
    
    return connInfo;
}

void ConnectionManager::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 701: throw std::logic_error("Incompatible protocol info");
        case 702: throw std::logic_error("Incompatible directions");
        case 703: throw std::logic_error("Insufficient network resources");
        case 704: throw std::logic_error("Local restrictions");
        case 705: throw std::logic_error("Access denied");
        case 706: throw std::logic_error("Invalid connection reference");
        case 707: throw std::logic_error("Managers are not part of the same network");
        
        default: upnp::handleUPnPResult(errorCode);
    }
}

ConnectionManagerAction ConnectionManager::actionFromString(const std::string& action)
{
    if (action == "GetProtocolInfo")            return ConnectionManagerAction::GetProtocolInfo;
    if (action == "PrepareForConnection")       return ConnectionManagerAction::PrepareForConnection;
    if (action == "ConnectionComplete")         return ConnectionManagerAction::ConnectionComplete;
    if (action == "GetCurrentConnectionIDs")    return ConnectionManagerAction::GetCurrentConnectionIDs;
    if (action == "GetCurrentConnectionInfo")   return ConnectionManagerAction::GetCurrentConnectionInfo;
    
    throw std::logic_error("Unknown ConnectionManager action:" + action);
}

std::string ConnectionManager::actionToString(ConnectionManagerAction action)
{
    switch (action)
    {
        case ConnectionManagerAction::GetProtocolInfo:                return "GetProtocolInfo";
        case ConnectionManagerAction::PrepareForConnection:           return "PrepareForConnection";
        case ConnectionManagerAction::ConnectionComplete:             return "ConnectionComplete";
        case ConnectionManagerAction::GetCurrentConnectionIDs:        return "GetCurrentConnectionIDs";
        case ConnectionManagerAction::GetCurrentConnectionInfo:       return "GetCurrentConnectionInfo";

        default: throw std::logic_error("Unknown ConnectionManager action");
    }
}

ConnectionManagerVariable ConnectionManager::variableFromString(const std::string& var)
{
    if (var == "SourceProtocolInfo")            return ConnectionManagerVariable::SourceProtocolInfo;
    if (var == "SinkProtocolInfo")              return ConnectionManagerVariable::SinkProtocolInfo;
    if (var == "CurrentConnectionIDs")          return ConnectionManagerVariable::CurrentConnectionIds;
    if (var == "A_ARG_TYPE_ConnectionStatus")   return ConnectionManagerVariable::ArgumentTypeConnectionStatus;
    if (var == "A_ARG_TYPE_ConnectionManager")  return ConnectionManagerVariable::ArgumentTypeConnectionManager;
    if (var == "A_ARG_TYPE_Direction")          return ConnectionManagerVariable::ArgumentTypeDirection;
    if (var == "A_ARG_TYPE_ProtocolInfo")       return ConnectionManagerVariable::ArgumentTypeProtocolInfo;
    if (var == "A_ARG_TYPE_ConnectionID")       return ConnectionManagerVariable::ArgumentTypeConnectionId;
    if (var == "A_ARG_TYPE_AVTransportID")      return ConnectionManagerVariable::ArgumentTypeAVTransportId;
    if (var == "A_ARG_TYPE_RcsID")              return ConnectionManagerVariable::ArgumentTypeRecourceId;
    
    throw std::logic_error("Unknown ConnectionManager variable:" + var);
}

std::string ConnectionManager::variableToString(ConnectionManagerVariable var)
{
    switch (var)
    {
        case ConnectionManagerVariable::SourceProtocolInfo:             return "SourceProtocolInfo";
        case ConnectionManagerVariable::SinkProtocolInfo:               return "SinkProtocolInfo";
        case ConnectionManagerVariable::CurrentConnectionIds:           return "CurrentConnectionIDs";
        case ConnectionManagerVariable::ArgumentTypeConnectionStatus:   return "A_ARG_TYPE_ConnectionStatus";
        case ConnectionManagerVariable::ArgumentTypeConnectionManager:  return "A_ARG_TYPE_ConnectionManager";
        case ConnectionManagerVariable::ArgumentTypeDirection:          return "A_ARG_TYPE_Direction";
        case ConnectionManagerVariable::ArgumentTypeProtocolInfo:       return "A_ARG_TYPE_ProtocolInfo";
        case ConnectionManagerVariable::ArgumentTypeConnectionId:       return "A_ARG_TYPE_ConnectionID";
        case ConnectionManagerVariable::ArgumentTypeAVTransportId:      return "A_ARG_TYPE_AVTransportID";
        case ConnectionManagerVariable::ArgumentTypeRecourceId:         return "A_ARG_TYPE_RcsID";
            
        default: throw std::logic_error("Unknown ConnectionManager action");
    }
}

}
