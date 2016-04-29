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
    { "GetProtocolInfo",             Action::GetProtocolInfo },
    { "PrepareForConnection",        Action::PrepareForConnection },
    { "ConnectionComplete",          Action::ConnectionComplete },
    { "GetCurrentConnectionIDs",     Action::GetCurrentConnectionIDs },
    { "GetCurrentConnectionInfo",    Action::GetCurrentConnectionInfo },
    { "GetRendererItemInfo",         Action::GetRendererItemInfo },
    { "GetFeatureList",              Action::GetFeatureList },
}};

static constexpr EnumMap<Variable> s_variableNames {{
    { "SourceProtocolInfo",            Variable::SourceProtocolInfo },
    { "SinkProtocolInfo",              Variable::SinkProtocolInfo },
    { "CurrentConnectionIDs",          Variable::CurrentConnectionIds },
    { "A_ARG_TYPE_ConnectionStatus",   Variable::ArgumentTypeConnectionStatus },
    { "A_ARG_TYPE_ConnectionManager",  Variable::ArgumentTypeConnectionManager },
    { "A_ARG_TYPE_Direction",          Variable::ArgumentTypeDirection },
    { "A_ARG_TYPE_ProtocolInfo",       Variable::ArgumentTypeProtocolInfo },
    { "A_ARG_TYPE_ConnectionID",       Variable::ArgumentTypeConnectionId },
    { "A_ARG_TYPE_AVTransportID",      Variable::ArgumentTypeAVTransportId },
    { "A_ARG_TYPE_RcsID",              Variable::ArgumentTypeRecourceId },
    { "A_ARG_TYPE_ItemInfoFilter",     Variable::ArgumentTypeItemInfoFilter },
    { "A_ARG_TYPE_Result",             Variable::ArgumentTypeResult },
    { "A_ARG_TYPE_RenderingInfoList",  Variable::ArgumentTypeRenderingInfoList }
}};

static constexpr EnumMap<ConnectionStatus> s_connStatusNames {{
    { "OK",                     ConnectionStatus::Ok },
    { "ContentFormatMismatch",  ConnectionStatus::ContentFormatMismatch },
    { "InsufficientBandwith",   ConnectionStatus::InsufficientBandwith },
    { "UnreliableChannel",      ConnectionStatus::UnreliableChannel }
}};

static constexpr EnumMap<Direction> s_directionNames {{
    { "Input",  Direction::Input},
    { "Output", Direction::Output}
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
    return string_cast(value);
}

Variable ConnectionManager::variableFromString(gsl::cstring_span<> value)
{
    return enum_cast<Variable>(value);
}

const char* ConnectionManager::toString(Variable var) noexcept
{
    return string_cast(var);
}

ConnectionStatus ConnectionManager::connectionStatusFromString(gsl::cstring_span<> value)
{
    return enum_cast<ConnectionStatus>(value);
}

const char* ConnectionManager::toString(ConnectionStatus value) noexcept
{
    return string_cast(value);
}

Direction ConnectionManager::directionFromString(gsl::cstring_span<> value)
{
    return enum_cast<Direction>(value);
}

const char* ConnectionManager::toString(Direction value) noexcept
{
    return string_cast(value);
}

}

