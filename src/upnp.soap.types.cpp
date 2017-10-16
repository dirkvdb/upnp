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

#include "upnp/upnp.soap.types.h"

#include "upnp.soap.parseutils.h"

namespace upnp
{
namespace soap
{

Fault::Fault(uint32_t ec)
: m_errorCode(ec)
{
}

Fault::Fault(uint32_t ec, std::string desc)
: m_errorCode(ec)
, m_errorDescription(std::move(desc))
{
}

uint32_t Fault::errorCode() const noexcept
{
    return m_errorCode;
}

const std::string& Fault::errorDescription() const noexcept
{
    return m_errorDescription;
}

const char* Fault::what() const noexcept
{
    return m_errorDescription.c_str();
}

bool Fault::operator==(const Fault& other) const noexcept
{
    return m_errorCode == other.m_errorCode && m_errorDescription == other.m_errorDescription;
}

ActionResult::ActionResult(http::StatusCode sc, std::string res)
: httpStatus(sc)
, response(std::move(res))
{
}

bool ActionResult::isFaulty() const noexcept
{
    return httpStatus == http::StatusCode::InternalServerError;
}

Fault ActionResult::getFault() const
{
    if (!isFaulty())
    {
        throw std::runtime_error("No soap fault available for parsing");
    }

    return soap::parseFault(response);
}

bool ActionResult::operator==(const ActionResult& other) const noexcept
{
    return httpStatus == other.httpStatus && response == other.response;
}

}
}
