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

#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "utils/stringoperations.h"

namespace upnp
{

inline void throwOnNull(const void* pPtr, const char* pMsg)
{
    if (!pPtr)
    {
        throw std::runtime_error(pMsg);
    }
}

inline std::string durationToString(std::chrono::seconds duration)
{
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration - minutes);

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << hours.count() << ':'
       << std::setw(2) << std::setfill('0') << minutes.count() << ':'
       << std::setw(2) << std::setfill('0') << seconds.count();
    return ss.str();
}

inline std::chrono::seconds durationFromString(const std::string& durationString)
{
    auto times = utils::stringops::tokenize(durationString, ":");

    if (times.size() != 3)
    {
        throw std::runtime_error("Invalid duration format: " + durationString);
    }

    std::chrono::hours hours(0);
    if (times[0].size() > 0)
    {
        hours = std::chrono::hours(utils::stringops::toNumeric<uint32_t>(times[0]));
    }

    std::chrono::minutes minutes(utils::stringops::toNumeric<uint32_t>(times[1]));

    auto secondsAndFractals = utils::stringops::tokenize(times[2], ".");
    std::chrono::seconds seconds(utils::stringops::toNumeric<uint32_t>(secondsAndFractals.front()));

    if (secondsAndFractals.size() > 1)
    {
        // process fractal part here
    }

    return hours + minutes + seconds;
}

template <typename DestView, typename SourceView>
DestView sv_cast(const SourceView& sv)
{
    return DestView(sv.data(), sv.size());
}

}
