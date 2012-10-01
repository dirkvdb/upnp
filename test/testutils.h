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

#ifndef UPNP_TEST_UTILS_H
#define UPNP_TEST_UTILS_H

#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"

namespace upnp
{

inline xml::Document generateActionResponse(const std::string& action, ServiceType type, const std::vector<std::pair<std::string, std::string>>& vars = {})
{
    std::stringstream ss;
    ss << "<u:" << action << "Response xmlns:u=\"" << serviceTypeToUrnString(type) << "\"></u:" << action << "Response>";
    
    return xml::Document(ss.str());
}

}

#endif
