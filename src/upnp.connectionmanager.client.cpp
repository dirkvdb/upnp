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

#include "upnp/upnp.connectionmanager.client.h"
#include "upnp.connectionmanager.typeconversions.h"

#include "upnp/upnp.device.h"
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

static const std::chrono::seconds s_subscriptionTimeout(1801);

ServiceType::Type ServiceTraits::SvcType = ServiceType::ConnectionManager;

Action ServiceTraits::actionFromString(const std::string& action)
{
    return ConnectionManager::actionFromString(action);
}

const char* ServiceTraits::actionToString(Action action)
{
    return ConnectionManager::toString(action);
}

Variable ServiceTraits::variableFromString(const std::string& var)
{
    return ConnectionManager::variableFromString(var);
}

const char* ServiceTraits::variableToString(Variable var)
{
    return ConnectionManager::toString(var);
}

Client::Client(IClient2& client)
: ServiceClientBase(client)
{
}

void Client::getProtocolInfo(std::function<void(Status, std::vector<ProtocolInfo>)> cb)
{
    executeAction(Action::GetProtocolInfo, [=] (Status status, std::string response) {
        std::vector<ProtocolInfo> protocolInfo;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive | parse_trim_whitespace>(&response.front());
                auto& sink = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("Sink");

                auto infos = stringops::tokenize(sink.value_string(), ',');
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
                }
            }
            catch(std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, e.what());
            }
        }

        cb(status, protocolInfo);
    });
}

void Client::prepareForConnection(const ProtocolInfo& protocolInfo,
                                  const std::string& peerConnectionManager,
                                  int32_t peerConnectionId,
                                  Direction direction,
                                  std::function<void(Status, ConnectionInfo)> cb)
{
    executeAction(Action::PrepareForConnection, { {"RemoteProtocolInfo", protocolInfo.toString()},
                                                  {"PeerConnectionManager", peerConnectionManager},
                                                  {"PeerConnectionID", std::to_string(peerConnectionId)},
                                                  {"Direction", toString(direction)} }, [=] (Status status, const std::string& response) {
        ConnectionInfo connInfo;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& rootNode = doc.first_node_ref().first_node_ref().first_node_ref();

                connInfo.peerConnectionManager      = peerConnectionManager;
                connInfo.peerConnectionId           = peerConnectionId;
                connInfo.protocolInfo               = protocolInfo;
                connInfo.direction                  = direction;
                connInfo.connectionId               = std::stoi(rootNode.first_node_ref("ConnectionId").value());
                connInfo.avTransportId              = std::stoi(rootNode.first_node_ref("AVTransportID").value());
                connInfo.renderingControlServiceId  = std::stoi(rootNode.first_node_ref("RcsID").value());
            }
            catch(std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, e.what());
            }
        }

        cb(status, connInfo);
    });
}

void Client::connectionComplete(const ConnectionInfo& connectionInfo, std::function<void(Status)> cb)
{
    executeAction(Action::ConnectionComplete, { {"ConnectionID", std::to_string(connectionInfo.connectionId)} }, [cb] (Status status, const std::string&) {
        cb(status);
    });
}

void Client::getCurrentConnectionIds(std::function<void(Status, std::vector<std::string>)> cb)
{
    executeAction(Action::GetCurrentConnectionIDs, [cb] (Status status, const std::string& response) {
        std::vector<std::string> ids;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                ids = stringops::tokenize(doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("ConnectionIDs").value_string(), ',');
            }
            catch(std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, e.what());
            }
        }

        cb(status, ids);
    });
}

void Client::getCurrentConnectionInfo(int32_t connectionId, std::function<void(Status, ConnectionInfo)> cb)
{
    executeAction(Action::GetCurrentConnectionInfo, { {"ConnectionID", std::to_string(connectionId)} }, [=] (Status status, const std::string& response) {
        ConnectionInfo connInfo;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& rootNode = doc.first_node_ref().first_node_ref().first_node_ref();

                connInfo.connectionId               = connectionId;
                connInfo.avTransportId              = std::stoi(rootNode.first_node_ref("AVTransportID").value_string());
                connInfo.renderingControlServiceId  = std::stoi(rootNode.first_node_ref("RcsID").value_string());
                connInfo.protocolInfo               = ProtocolInfo(rootNode.first_node_ref("ProtocolInfo").value_string());
                connInfo.peerConnectionManager      = rootNode.first_node_ref("PeerConnectionManager").value_string();
                connInfo.peerConnectionId           = std::stoi(rootNode.first_node_ref("PeerConnectionID").value_string());
                connInfo.direction                  = directionFromString(rootNode.first_node_ref("Direction").value_view());
                connInfo.connectionStatus           = connectionStatusFromString(rootNode.first_node_ref("Status").value_view());
            }
            catch(std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, e.what());
            }
        }

        cb(status, connInfo);
    });
}

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return s_subscriptionTimeout;
}

// void Client::handleUPnPResult(int errorCode)
// {
//     if (errorCode == UPNP_E_SUCCESS) return;

//     switch (errorCode)
//     {
//         case 701: throw Exception(errorCode, "Incompatible protocol info");
//         case 702: throw Exception(errorCode, "Incompatible directions");
//         case 703: throw Exception(errorCode, "Insufficient network resources");
//         case 704: throw Exception(errorCode, "Local restrictions");
//         case 705: throw Exception(errorCode, "Access denied");
//         case 706: throw Exception(errorCode, "Invalid connection reference");
//         case 707: throw Exception(errorCode, "Managers are not part of the same network");

//         default: upnp::handleUPnPResult(errorCode);
//     }
// }

}
}
