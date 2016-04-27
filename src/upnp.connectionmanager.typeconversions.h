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

#include "upnp/upnp.connectionmanager.types.h"

namespace upnp
{
namespace ConnectionManager
{

Action actionFromString(const std::string& action);
const char* toString(Action action) noexcept;

Variable variableFromString(const std::string& var);
const char* toString(Variable var) noexcept;

ConnectionStatus connectionStatusFromString(const std::string& status);
const char* toString(ConnectionStatus status) noexcept;

Direction directionFromString(const std::string& direction);
const char* toString(Direction direction) noexcept;

}
}
