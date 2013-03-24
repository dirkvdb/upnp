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

#include "upnp/upnpconnectionmanagerclient.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

using namespace utils;


namespace upnp
{
namespace ConnectionManager
{

static const int32_t g_subscriptionTimeout = 1801;

Client::Client(IClient& client)
: ServiceBase(client)
{
}

std::vector<ProtocolInfo> Client::getProtocolInfo()
{
    xml::Document result = executeAction(Action::GetProtocolInfo);
    
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

ConnectionInfo Client::prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, int32_t peerConnectionId, Direction direction)
{
    xml::Document result = executeAction(Action::PrepareForConnection, { {"RemoteProtocolInfo", protocolInfo.toString()},
                                                                         {"PeerConnectionManager", peerConnectionManager},
                                                                         {"PeerConnectionID", std::to_string(peerConnectionId)},
                                                                         {"Direction", toString(direction)} });

    ConnectionInfo connInfo;
    connInfo.peerConnectionManager      = peerConnectionManager;
    connInfo.peerConnectionId           = peerConnectionId;
    connInfo.protocolInfo               = protocolInfo;
    connInfo.direction                  = direction;
    connInfo.connectionId               = std::stoi(result.getChildNodeValue("ConnectionID"));
    connInfo.avTransportId              = std::stoi(result.getChildNodeValue("AVTransportID"));
    connInfo.renderingControlServiceId  = std::stoi(result.getChildNodeValue("RcsID"));
    
    return connInfo;
}

void Client::connectionComplete(const ConnectionInfo& connectionInfo)
{
    executeAction(Action::ConnectionComplete, { {"ConnectionID", std::to_string(connectionInfo.connectionId)} });
}

std::vector<std::string> Client::getCurrentConnectionIds()
{
    xml::Document result = executeAction(Action::GetCurrentConnectionIDs);
    std::vector<std::string> ids = stringops::tokenize(result.getChildNodeValue("ConnectionIDs"), ",");
    
    return ids;
}

ServiceType Client::getType()
{
    return ServiceType::ConnectionManager;
}

int32_t Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

ConnectionInfo Client::getCurrentConnectionInfo(int32_t connectionId)
{
    xml::Document result = executeAction(Action::GetCurrentConnectionInfo, { {"ConnectionID", std::to_string(connectionId)} });
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = connectionId;
    connInfo.avTransportId              = std::stoi(result.getChildNodeValue("AVTransportID"));
    connInfo.renderingControlServiceId  = std::stoi(result.getChildNodeValue("RcsID"));
    connInfo.protocolInfo               = ProtocolInfo(result.getChildNodeValue("ProtocolInfo"));
    connInfo.peerConnectionManager      = result.getChildNodeValue("PeerConnectionManager");
    connInfo.peerConnectionId           = std::stoi(result.getChildNodeValue("PeerConnectionID"));
    connInfo.direction                  = directionFromString(result.getChildNodeValue("Direction"));
    connInfo.connectionStatus           = connectionStatusFromString(result.getChildNodeValue("Status"));
    
    return connInfo;
}

void Client::handleUPnPResult(int errorCode)
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

Action Client::actionFromString(const std::string& action)
{
    return ConnectionManager::actionFromString(action);
}

std::string Client::actionToString(Action action)
{
    return ConnectionManager::toString(action);
}

Variable Client::variableFromString(const std::string& var)
{
    return ConnectionManager::variableFromString(var);
}

std::string Client::variableToString(Variable var)
{
    return ConnectionManager::toString(var);
}

}
}
