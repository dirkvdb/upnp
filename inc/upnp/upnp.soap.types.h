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
#include <cinttypes>
#include <experimental/optional>

namespace upnp
{
namespace soap
{

struct Fault
{
    Fault() = default;
    Fault(uint32_t ec, std::string desc)
    : errorCode(ec)
    , errorDescription(std::move(desc))
    {
    }

    uint32_t errorCode = 0;
    std::string errorDescription;
};

struct ActionResult
{
    ActionResult(std::string cont) : contents(std::move(cont)) {}
    ActionResult(Fault flt) : fault(std::move(flt)) {}
    ActionResult(std::string cont, Fault flt) : contents(std::move(cont)), fault(std::move(flt)) {}

    std::string contents;
    std::experimental::optional<Fault> fault;
};

}
}
