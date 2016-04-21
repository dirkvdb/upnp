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

#include "rapidxml.hpp"

namespace upnp
{
namespace ConnectionManager
{

using namespace utils;
using namespace rapidxml_ns;
using namespace std::string_literals;

static const std::chrono::seconds g_subscriptionTimeout(1801);

Client::Client(Client2& client)
: ServiceClientBase(client)
{
}

void Client::getProtocolInfo(std::function<void(int32_t, std::vector<ProtocolInfo>)> cb)
{
    executeAction(Action::GetProtocolInfo, [=] (int32_t status, std::string response) {
        std::vector<ProtocolInfo> protocolInfo;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(&response.front());
                auto& sink = doc.first_node_ref("Envelope").first_node_ref("Body").first_node_ref("Sink");
                
                auto infos = stringops::tokenize(sink.value(), ',');
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
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }
        
        cb(status, protocolInfo);
    });
}

void Client::prepareForConnection(const ProtocolInfo& protocolInfo,
                                  const std::string& peerConnectionManager,
                                  int32_t peerConnectionId,
                                  Direction direction,
                                  std::function<void(int32_t, ConnectionInfo)> cb)
{
    executeAction(Action::PrepareForConnection, { {"RemoteProtocolInfo", protocolInfo.toString()},
                                                  {"PeerConnectionManager", peerConnectionManager},
                                                  {"PeerConnectionID", std::to_string(peerConnectionId)},
                                                  {"Direction", toString(direction)} }, [=] (int32_t status, const std::string& response) {
        ConnectionInfo connInfo;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");
            
                connInfo.peerConnectionManager      = peerConnectionManager;
                connInfo.peerConnectionId           = peerConnectionId;
                connInfo.protocolInfo               = protocolInfo;
                connInfo.direction                  = direction;
                connInfo.connectionId               = std::stoi(body.first_node_ref("ConnectionId").value());
                connInfo.avTransportId              = std::stoi(body.first_node_ref("AVTransportID").value());
                connInfo.renderingControlServiceId  = std::stoi(body.first_node_ref("RcsID").value());
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }
        
        cb(status, connInfo);
    });
}

void Client::connectionComplete(const ConnectionInfo& connectionInfo, std::function<void(int32_t)> cb)
{
    executeAction(Action::ConnectionComplete, { {"ConnectionID", std::to_string(connectionInfo.connectionId)} }, [cb] (int32_t status, const std::string&) {
        cb(status);
    });
}

void Client::getCurrentConnectionIds(std::function<void(int32_t, std::vector<std::string>)> cb)
{
    executeAction(Action::GetCurrentConnectionIDs, [cb] (int32_t status, const std::string& response) {
        std::vector<std::string> ids;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");
                ids = stringops::tokenize(body.first_node_ref("ConnectionIDs").value(), ',');
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }
        
        cb(status, ids);
    });
}

ServiceType Client::getType()
{
    return ServiceType::ConnectionManager;
}

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

void Client::getCurrentConnectionInfo(int32_t connectionId, std::function<void(int32_t, ConnectionInfo)> cb)
{
    executeAction(Action::GetCurrentConnectionInfo, { {"ConnectionID", std::to_string(connectionId)} }, [=] (int32_t status, const std::string& response) {
        ConnectionInfo connInfo;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");
    
                connInfo.connectionId               = connectionId;
                connInfo.avTransportId              = std::stoi(body.first_node_ref("AVTransportID").value());
                connInfo.renderingControlServiceId  = std::stoi(body.first_node_ref("RcsID").value());
                connInfo.protocolInfo               = ProtocolInfo(body.first_node_ref("ProtocolInfo").value());
                connInfo.peerConnectionManager      = body.first_node_ref("PeerConnectionManager").value();
                connInfo.peerConnectionId           = std::stoi(body.first_node_ref("PeerConnectionID").value());
                connInfo.direction                  = directionFromString(body.first_node_ref("Direction").value());
                connInfo.connectionStatus           = connectionStatusFromString(body.first_node_ref("Status").value());
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }
        
        cb(status, connInfo);
    });
}

void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 701: throw Exception(errorCode, "Incompatible protocol info");
        case 702: throw Exception(errorCode, "Incompatible directions");
        case 703: throw Exception(errorCode, "Insufficient network resources");
        case 704: throw Exception(errorCode, "Local restrictions");
        case 705: throw Exception(errorCode, "Access denied");
        case 706: throw Exception(errorCode, "Invalid connection reference");
        case 707: throw Exception(errorCode, "Managers are not part of the same network");
        
        default: upnp::handleUPnPResult(errorCode);
    }
}

Action Client::actionFromString(const std::string& action) const
{
    return ConnectionManager::actionFromString(action);
}

std::string Client::actionToString(Action action) const
{
    return ConnectionManager::toString(action);
}

Variable Client::variableFromString(const std::string& var) const
{
    return ConnectionManager::variableFromString(var);
}

std::string Client::variableToString(Variable var) const 
{
    return ConnectionManager::toString(var);
}

}
}
