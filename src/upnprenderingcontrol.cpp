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

#include "upnp/upnprenderingcontrol.h"

#include "upnp/upnputils.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

//static const char* ConnectionManagerServiceType = "urn:schemas-upnp-org:service:RenderingControl:1";

using namespace utils;

namespace upnp
{

RenderingControl::RenderingControl(const Client& client)
: m_Client(client)
{
}

void RenderingControl::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
}

}
