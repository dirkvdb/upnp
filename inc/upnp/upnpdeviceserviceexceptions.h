//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include <string>
#include <map>
#include <chrono>
#include <cinttypes>

#include "upnp/upnptypes.h"
#include "upnp/upnp.soap.types.h"

namespace upnp
{

#define DEFINE_UPNP_SERVICE_FAULT(name, code, msg) \
class name : public soap::Fault \
{ \
public: \
    name() : soap::Fault(code, msg) {} \
}; \

DEFINE_UPNP_SERVICE_FAULT(InvalidAction,               401, "Invalid action")
DEFINE_UPNP_SERVICE_FAULT(InvalidArgumentsService,     401, "Invalid arguments")
DEFINE_UPNP_SERVICE_FAULT(InvalidSubscriptionId,       401, "Invalid subscription id")
DEFINE_UPNP_SERVICE_FAULT(ActionFailed,                501, "Action failed")

}
