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
#include <curl/curl.h>

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

    void setTimeout(std::chrono::milliseconds timeout) noexcept;

    void getContentLength(const std::string& url, std::function<void(int32_t, size_t)> cb);
    void get(const std::string& url, std::function<void(int32_t, std::string)> cb);
    void get(const std::string& url, std::function<void(int32_t, std::vector<uint8_t>)> cb);
    void get(const std::string& url, uint8_t* data, std::function<void(int32_t, uint8_t*)> cb);

    void getRange(const std::string& url, uint64_t offset, uint64_t size, std::function<void(int32_t, std::vector<uint8_t>)> cb);
    void getRange(const std::string& url, uint64_t offset, uint64_t size, uint8_t* pData, std::function<void(int32_t, uint8_t*)> cb);

    static const char* errorToString(int32_t errorCode);

private:
    static int handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);

    uv::Loop& m_loop;
    uv::Timer m_timer;
    uint32_t m_timeout;
    CURLM* m_multiHandle;
};

}
}
