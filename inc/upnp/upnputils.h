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
#include <iomanip>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "utils/stringoperations.h"

namespace upnp
{

inline void throwOnNull(const void* pPtr, const char* pMsg)
{
    if (!pPtr)
    {
        throw std::logic_error(pMsg);
    }
}

inline void handleUPnPResult(int errorCode)
{
    if (UPNP_E_SUCCESS == errorCode)
    {
        return;
    }
    else if (UPNP_E_SOCKET_CONNECT == errorCode)
    {
        throw std::logic_error("Could not connect to the device");
    }
    else if (UPNP_E_BAD_RESPONSE == errorCode)
    {
        throw std::logic_error("Bad response from the server");
    }
    else if (401 == errorCode)
    {
        throw std::logic_error("Failed to connect");
    }
    else if (500 == errorCode)
    {
        throw std::logic_error("UPnP device error");
    }
    else if (501 == errorCode)
    {
        throw std::logic_error("Request not supported");
    }
    else if (801 == errorCode)
    {
        throw std::logic_error("UPnP Access denied");
    }
    else
    {
        std::stringstream ss;
        ss << UpnpGetErrorMessage(errorCode) << " (" << errorCode << ")";
        throw std::logic_error(ss.str().c_str());
    }
}

inline std::string durationToString(uint32_t durationInSecs)
{
    uint32_t hours = durationInSecs / 3600;
    durationInSecs -= hours * 3600;

    uint32_t minutes = durationInSecs / 60;
    uint32_t seconds = durationInSecs % 60;


    std::stringstream ss;

    ss << std::setw(2) << std::setfill('0') << hours << ':'
    << std::setw(2) << std::setfill('0') << minutes << ':'
    << std::setw(2) << std::setfill('0') << seconds;
    return ss.str();
}

inline uint32_t durationFromString(const std::string& durationString)
{
    uint32_t duration = 0;
    auto times = utils::stringops::tokenize(durationString, ":");

    if (times.size() != 3)
    {
        throw std::runtime_error("Invalid duration format: " + durationString);
    }

    uint32_t hours = 0;
    if (times[0].size() > 0)
    {
        hours = utils::stringops::toNumeric<uint32_t>(times[0]);
    }

    uint32_t minutes = utils::stringops::toNumeric<uint32_t>(times[1]);

    auto secondsAndFractals = utils::stringops::tokenize(times[2], ".");
    uint32_t seconds = utils::stringops::toNumeric<uint32_t>(secondsAndFractals.front());

    if (secondsAndFractals.size() > 1)
    {
        // process fractal part here
    }

    duration += hours * 3600;
    duration += minutes * 60;
    duration += seconds;
    return duration;}
}


#endif
