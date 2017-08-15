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

#include "upnp/upnp.http.types.h"

#include <string>
#include <string_view>
#include <system_error>
#include <boost/utility/string_view.hpp>

namespace upnp
{
namespace http
{

const char* okResponse();
std::string createResponse(StatusCode status, const std::string& body);

template <typename DestView, typename SourceView>
DestView sv_cast(SourceView sv)
{
    return DestView(sv.data(), sv.size());
}

}
}
