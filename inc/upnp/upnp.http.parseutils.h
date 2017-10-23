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

#include "utils/stringoperations.h"

#include <cinttypes>
#include <string_view>

namespace upnp
{
namespace http
{
namespace parseutils
{

struct Range
{
    uint64_t start = 0;
    uint64_t end = 0;
};

Range parseRange(std::string_view range)
{
    Range result;

    if (!utils::stringops::startsWith(range, "bytes="))
    {
        throw std::invalid_argument("Invalid range header: " + std::string(range));
    }

    auto split = utils::stringops::tokenize(&range[6], '-');
    if (split.size() > 2)
    {
        throw std::invalid_argument("Invalid range header: " + std::string(range));
    }

    if (split.size() == 1)
    {
        if (range[range.size() - 1] != '-')
        {
            throw std::invalid_argument("Invalid range header: " + std::string(range));
        }
    }
    else if (!split[1].empty())
    {
        result.end = std::stoul(split[1]);
    }

    if (!split[0].empty())
    {
        result.start = std::stoul(split[0]);
    }

    return result;
}

}
}
}

