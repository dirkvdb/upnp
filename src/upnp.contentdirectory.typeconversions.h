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

#include "upnp/upnp.item.h"
#include "upnp/upnp.contentdirectory.types.h"

#include <vector>
#include <cinttypes>
#include <string_view>

namespace upnp
{
namespace ContentDirectory
{

Action actionFromString(std::string_view data);
const char* actionToString(Action value) noexcept;

Variable variableFromString(std::string_view data);
const char* variableToString(Variable value) noexcept;

BrowseFlag browseFlagFromString(std::string_view data);
std::string browseFlagToString(BrowseFlag browseFlag) noexcept;

SortType sortTypeFromString(char c);

}
}
