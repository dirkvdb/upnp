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

#ifndef UPNP_UTILS_H
#define UPNP_UTILS_H

#include <string>
#include <stdexcept>
#include <sstream>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

namespace upnp
{

inline void handleUPnPResult(int errorCode)
{
    if (UPNP_E_SUCCESS == errorCode)
    {
        return;
    }
    else if (708 == errorCode)
    {
        throw std::logic_error("Unsupported or invalid search criteria");
    }
    else if (709 == errorCode)
    {
        throw std::logic_error("Unsupported or invalid sort criteria");
    }
    else if (801 == errorCode)
    {
        throw std::logic_error("UPnP Access denied");
    }
    else
    {
        throw std::logic_error(UpnpGetErrorMessage(errorCode));
    }
}

}


#endif
