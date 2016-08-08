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

#include "upnp.http.utils.h"

#include "utils/format.h"
#include "upnp.enumutils.h"

namespace upnp
{
namespace http
{

static const char* s_response =
    "HTTP/1.1 {} {}\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.1\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
    "\r\n"
    "{}";

static const char* s_okResponse =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: 0\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n";

const char* okResponse()
{
    return s_okResponse;
}

std::string createResponse(StatusCode status, const std::string& body)
{
    return fmt::format(s_response, enum_value(status), status_message(status), body.size(), body);
}

}
}
