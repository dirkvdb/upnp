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

#include "upnp/upnp.avtransport.types.h"
#include "gsl/span.h"

namespace upnp
{
namespace AVTransport
{

Action actionFromString(gsl::span<const char> value);
const char* actionToString(Action action);
Variable variableFromString(gsl::span<const char> value);
const char* variableToString(Variable var);
State stateFromString(gsl::span<const char> value);
PlaylistType playlistTypeFromString(gsl::span<const char> value);
PlaylistStep playlistStepFromString(gsl::span<const char> value);
const char* toString(State state);
const char* toString(CurrentMediaCategory state);
const char* toString(DRMState state);
const char* toString(PlaylistStep step);
const char* toString(PlaylistType type);
const char* toString(PlaylistState state);
Status statusFromString(gsl::span<const char> value);
const char* toString(Status status);
SeekMode seekModeFromString(gsl::span<const char> value);
const char* toString(SeekMode mode);
PlayMode playModeFromString(gsl::span<const char> value);
const char* toString(PlayMode mode);

}
}
