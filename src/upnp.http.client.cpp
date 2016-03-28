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

#include "upnp.http.client.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include "utils/format.h"

using namespace utils;

namespace upnp
{
namespace http
{

Client::Client(uv::Loop& loop)
: m_socket(loop)
{
}

size_t Client::getContentLength(const std::string& url)
{
    return 0;
}

std::string Client::getText(const std::string& url)
{
    return url;
}

std::vector<uint8_t> Client::getData(const std::string& url)
{
    return {};
}

std::vector<uint8_t> Client::getData(const std::string& url, uint64_t offset, uint64_t size)
{
    return {};
}

void Client::getData(const std::string& url, uint8_t* pData)
{
}

void Client::getData(const std::string& url, uint8_t* pData, uint64_t offset, uint64_t size)
{
}

}
}
