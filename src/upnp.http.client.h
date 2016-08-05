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

#include "URI.h"
#include "upnp/upnp.http.parser.h"
#include "upnp/upnp.http.types.h"
#include "upnp.http.utils.h"

#include <asio.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <cinttypes>
#include <unordered_map>
#include <experimental/string_view>

namespace upnp
{
namespace http
{

class Client
{
public:
    Client(asio::io_service& io);
    Client(const Client&) = delete;

    void setTimeout(std::chrono::milliseconds timeout) noexcept;
    void addHeader(std::string header);
    void setUrl(const std::string& url);

    const std::string& getResponseHeaderValue(const char* heeaderValue);
    std::string getResponseBody();
    uint32_t getStatus();

    void perform(Method method, std::function<void(const std::error_code&, Response response)> cb);
    void perform(Method method, const std::string& body, std::function<void(const std::error_code&, std::string data)> cb);
    void perform(Method method, uint8_t* data, std::function<void(const std::error_code&, StatusCode, uint8_t* data)> cb);

    void reset();

private:
    void checkTimeout(const asio::error_code& ec);

    void setMethodType(Method method);
    void performRequest(const asio::ip::tcp::endpoint& addr, std::function<void(const asio::error_code&)> cb);
    void performRequest(const asio::ip::tcp::endpoint& addr, const std::string& body, std::function<void(const asio::error_code&)> cb);
    void performRequest(const asio::ip::tcp::endpoint& addr, const std::vector<asio::const_buffer>& buffers, std::function<void(const asio::error_code&)> cb);
    void receiveData(std::function<void(const std::error_code&)> cb);

    URI m_uri;
    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    std::chrono::milliseconds m_timeout;
    std::string m_request;
    std::vector<std::string> m_headers;
    std::array<char, 2048> m_buffer;
    Parser m_parser;
};

}
}

namespace std
{

error_code make_error_code(upnp::http::error::ErrorCode e) noexcept;

}

