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

#ifndef UPNP_CONNECTION_MANAGER_TYPES_H
#define UPNP_CONNECTION_MANAGER_TYPES_H

namespace upnp
{
namespace ConnectionManager
{

static std::string UnknownConnectionId = "-1";
static std::string DefaultConnectionId = "0";


enum class Action
{
    GetProtocolInfo,
    PrepareForConnection, // Optional
    ConnectionComplete, // Optional
    GetCurrentConnectionIDs,
    GetCurrentConnectionInfo
};

enum class Variable
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

inline Action actionFromString(const std::string& action)
{
    if (action == "GetProtocolInfo")            return Action::GetProtocolInfo;
    if (action == "PrepareForConnection")       return Action::PrepareForConnection;
    if (action == "ConnectionComplete")         return Action::ConnectionComplete;
    if (action == "GetCurrentConnectionIDs")    return Action::GetCurrentConnectionIDs;
    if (action == "GetCurrentConnectionInfo")   return Action::GetCurrentConnectionInfo;
    
    throw std::logic_error("Unknown ConnectionManager action:" + action);
}

inline std::string actionToString(Action action)
{
    switch (action)
    {
        case Action::GetProtocolInfo:                return "GetProtocolInfo";
        case Action::PrepareForConnection:           return "PrepareForConnection";
        case Action::ConnectionComplete:             return "ConnectionComplete";
        case Action::GetCurrentConnectionIDs:        return "GetCurrentConnectionIDs";
        case Action::GetCurrentConnectionInfo:       return "GetCurrentConnectionInfo";

        default: throw std::logic_error("Unknown ConnectionManager action");
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "SourceProtocolInfo")            return Variable::SourceProtocolInfo;
    if (var == "SinkProtocolInfo")              return Variable::SinkProtocolInfo;
    if (var == "CurrentConnectionIDs")          return Variable::CurrentConnectionIds;
    if (var == "A_ARG_TYPE_ConnectionStatus")   return Variable::ArgumentTypeConnectionStatus;
    if (var == "A_ARG_TYPE_ConnectionManager")  return Variable::ArgumentTypeConnectionManager;
    if (var == "A_ARG_TYPE_Direction")          return Variable::ArgumentTypeDirection;
    if (var == "A_ARG_TYPE_ProtocolInfo")       return Variable::ArgumentTypeProtocolInfo;
    if (var == "A_ARG_TYPE_ConnectionID")       return Variable::ArgumentTypeConnectionId;
    if (var == "A_ARG_TYPE_AVTransportID")      return Variable::ArgumentTypeAVTransportId;
    if (var == "A_ARG_TYPE_RcsID")              return Variable::ArgumentTypeRecourceId;
    
    throw std::logic_error("Unknown ConnectionManager variable:" + var);
}

inline std::string variableToString(Variable var)
{
    switch (var)
    {
        case Variable::SourceProtocolInfo:             return "SourceProtocolInfo";
        case Variable::SinkProtocolInfo:               return "SinkProtocolInfo";
        case Variable::CurrentConnectionIds:           return "CurrentConnectionIDs";
        case Variable::ArgumentTypeConnectionStatus:   return "A_ARG_TYPE_ConnectionStatus";
        case Variable::ArgumentTypeConnectionManager:  return "A_ARG_TYPE_ConnectionManager";
        case Variable::ArgumentTypeDirection:          return "A_ARG_TYPE_Direction";
        case Variable::ArgumentTypeProtocolInfo:       return "A_ARG_TYPE_ProtocolInfo";
        case Variable::ArgumentTypeConnectionId:       return "A_ARG_TYPE_ConnectionID";
        case Variable::ArgumentTypeAVTransportId:      return "A_ARG_TYPE_AVTransportID";
        case Variable::ArgumentTypeRecourceId:         return "A_ARG_TYPE_RcsID";
            
        default: throw std::logic_error("Unknown ConnectionManager action");
    }
}
    
}
}

#endif