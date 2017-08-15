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

#include "upnp/upnp.http.types.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace http;

static constexpr EnumMap<http::Method> s_methodConv {{
    std::make_tuple("NOTIFY",       http::Method::Notify),
    std::make_tuple("MSEARCH",      http::Method::Search),

    std::make_tuple("SUBSCRIBE",    http::Method::Subscribe),
    std::make_tuple("UNSUBSCRIBE",  http::Method::Unsubscribe),

    std::make_tuple("GET",          http::Method::Get),
    std::make_tuple("HEAD",         http::Method::Head),
    std::make_tuple("POST",         http::Method::Post),
}};

ADD_ENUM_MAP(http::Method, s_methodConv)

namespace http
{

Method methodFromString(const std::string_view& str)
{
    return enum_cast<Method>(str);
}

Response::Response(StatusCode s)
: status(s)
{
}

Response::Response(beast::http::status s)
: Response(StatusCode(enum_value(s)))
{
}

Response::Response(StatusCode s, std::string b)
: status(s)
, body(std::move(b))
{
}

Response::Response(beast::http::status s, std::string b)
: Response(StatusCode(enum_value(s)), std::move(b))
{
}

bool Response::operator==(const Response& other) const noexcept
{
    return status == other.status && body == other.body;
}

}
}