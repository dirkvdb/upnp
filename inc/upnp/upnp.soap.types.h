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
#include <optional>

#include "upnp/upnp.http.types.h"

namespace upnp
{
namespace soap
{

class Fault : public std::exception
{
public:
    Fault(uint32_t ec);
    Fault(uint32_t ec, std::string desc);

    uint32_t errorCode() const noexcept;
    const std::string& errorDescription() const noexcept;

    const char* what() const noexcept override;

    bool operator==(const Fault& other) const noexcept;

private:
    uint32_t m_errorCode = 0;
    std::string m_errorDescription;
};

struct ActionResult
{
    ActionResult() = default;
    ActionResult(http::Response res) : response(std::move(res)) {}
    ActionResult(Fault flt) : fault(std::move(flt)) {}
    ActionResult(http::Response res, Fault flt) : response(std::move(res)), fault(std::move(flt)) {}

    http::Response response;
    std::optional<Fault> fault;
};

}
}
