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
#include <vector>
#include <cinttypes>

#include "upnp/upnpuv.h"

namespace upnp
{
namespace http
{

class Client
{
public:
    Client(uv::Loop& loop);
    Client(const Client&) = delete;

    size_t getContentLength(const std::string& url);
    std::string getText(const std::string& url);

    std::vector<uint8_t> getData(const std::string& url);
    std::vector<uint8_t> getData(const std::string& url, uint64_t offset, uint64_t size);

    void getData(const std::string& url, uint8_t* pData);
    void getData(const std::string& url, uint8_t* pData, uint64_t offset, uint64_t size);

private:
    uv::socket::Tcp m_socket;
};

}
}
