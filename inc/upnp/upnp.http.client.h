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

#include "upnp/upnp.uv.h"

namespace upnp
{
namespace http
{

class Client
{
public:
    Client(uv::Loop& loop);
    Client(const Client&) = delete;
    ~Client() noexcept;

    void setTimeout(std::chrono::milliseconds timeout) noexcept;

    void getContentLength(const std::string& url, std::function<void(int32_t status, size_t length)> cb);
    void get(const std::string& url, std::function<void(int32_t status, std::string data)> cb);
    void get(const std::string& url, std::function<void(int32_t status, std::vector<uint8_t> data)> cb);
    void get(const std::string& url, uint8_t* data, std::function<void(int32_t status, uint8_t* data)> cb);

    void getRange(const std::string& url, uint64_t offset, uint64_t size, std::function<void(int32_t status, std::vector<uint8_t> data)> cb);
    void getRange(const std::string& url, uint64_t offset, uint64_t size, uint8_t* pData, std::function<void(int32_t status, uint8_t* data)> cb);

    void subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                   std::function<void(int32_t status, std::string subId, std::chrono::seconds timeout, std::string response)> cb);
    void renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                           std::function<void(int32_t status, std::string subId, std::chrono::seconds timeout, std::string response)> cb);
    void unsubscribe(const std::string& url, const std::string& sid, std::function<void(int32_t status, std::string response)> cb);

    void soapAction(const std::string& url,
                    const std::string& actionName,
                    const std::string& serviceName,
                    const std::string& envelope,
                    std::function<void(int32_t status, std::string data)> cb);

    // Should only be called with negative error codes
    static std::string errorToString(int32_t errorCode);

private:
    static int handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);

    uv::Loop& m_loop;
    uv::Timer m_timer;
    uint32_t m_timeout;
    CURLM* m_multiHandle;
};

}
}
