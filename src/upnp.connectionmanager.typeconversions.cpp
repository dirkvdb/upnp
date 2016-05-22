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

#include "upnp.connectionmanager.typeconversions.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace ConnectionManager;

static constexpr EnumMap<Action> s_actionNames {{
    std::make_tuple("GetProtocolInfo",             Action::GetProtocolInfo),
    std::make_tuple("PrepareForConnection",        Action::PrepareForConnection),
    std::make_tuple("ConnectionComplete",          Action::ConnectionComplete),
    std::make_tuple("GetCurrentConnectionIDs",     Action::GetCurrentConnectionIDs),
    std::make_tuple("GetCurrentConnectionInfo",    Action::GetCurrentConnectionInfo),
    std::make_tuple("GetRendererItemInfo",         Action::GetRendererItemInfo),
    std::make_tuple("GetFeatureList",              Action::GetFeatureList),
}};

static constexpr EnumMap<Variable> s_variableNames {{
    std::make_tuple("SourceProtocolInfo",            Variable::SourceProtocolInfo),
    std::make_tuple("SinkProtocolInfo",              Variable::SinkProtocolInfo),
    std::make_tuple("CurrentConnectionIDs",          Variable::CurrentConnectionIds),
    std::make_tuple("A_ARG_TYPE_ConnectionStatus",   Variable::ArgumentTypeConnectionStatus),
    std::make_tuple("A_ARG_TYPE_ConnectionManager",  Variable::ArgumentTypeConnectionManager),
    std::make_tuple("A_ARG_TYPE_Direction",          Variable::ArgumentTypeDirection),
    std::make_tuple("A_ARG_TYPE_ProtocolInfo",       Variable::ArgumentTypeProtocolInfo),
    std::make_tuple("A_ARG_TYPE_ConnectionID",       Variable::ArgumentTypeConnectionId),
    std::make_tuple("A_ARG_TYPE_AVTransportID",      Variable::ArgumentTypeAVTransportId),
    std::make_tuple("A_ARG_TYPE_RcsID",              Variable::ArgumentTypeRecourceId),
    std::make_tuple("A_ARG_TYPE_ItemInfoFilter",     Variable::ArgumentTypeItemInfoFilter),
    std::make_tuple("A_ARG_TYPE_Result",             Variable::ArgumentTypeResult),
    std::make_tuple("A_ARG_TYPE_RenderingInfoList",  Variable::ArgumentTypeRenderingInfoList),
}};

static constexpr EnumMap<ConnectionStatus> s_connStatusNames {{
    std::make_tuple("OK",                     ConnectionStatus::Ok),
    std::make_tuple("ContentFormatMismatch",  ConnectionStatus::ContentFormatMismatch),
    std::make_tuple("InsufficientBandwith",   ConnectionStatus::InsufficientBandwith),
    std::make_tuple("UnreliableChannel",      ConnectionStatus::UnreliableChannel),
}};

static constexpr EnumMap<Direction> s_directionNames {{
    std::make_tuple("Input",  Direction::Input),
    std::make_tuple("Output", Direction::Output),
}};

ADD_ENUM_MAP(Action, s_actionNames)
ADD_ENUM_MAP(Variable, s_variableNames)
ADD_ENUM_MAP(ConnectionStatus, s_connStatusNames)
ADD_ENUM_MAP(Direction, s_directionNames)

Action ConnectionManager::actionFromString(gsl::cstring_span<> value)
{
    return enum_cast<Action>(value);
}

const char* ConnectionManager::toString(Action value) noexcept
{
    return enum_string(value);
}

Variable ConnectionManager::variableFromString(gsl::cstring_span<> value)
{
    return enum_cast<Variable>(value);
}

const char* ConnectionManager::toString(Variable var) noexcept
{
    return enum_string(var);
}

ConnectionStatus ConnectionManager::connectionStatusFromString(gsl::cstring_span<> value)
{
    return enum_cast<ConnectionStatus>(value);
}

const char* ConnectionManager::toString(ConnectionStatus value) noexcept
{
    return enum_string(value);
}

Direction ConnectionManager::directionFromString(gsl::cstring_span<> value)
{
    return enum_cast<Direction>(value);
}

const char* ConnectionManager::toString(Direction value) noexcept
{
    return enum_string(value);
}

}

