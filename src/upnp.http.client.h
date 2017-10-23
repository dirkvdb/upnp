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
#include "upnp/asio.h"
#include "upnp/upnp.http.types.h"
#include "upnp/upnp.coroutine.h"

#include <string>
#include <vector>
#include <chrono>
#include <cinttypes>
#include <experimental/coroutine>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <string_view>

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
    void addHeader(const std::string& name, const std::string& value);
    void setUrl(const std::string& url);

    std::string_view getResponseHeaderValue(const char* heeaderValue);
    std::string getResponseBody();
    uint32_t getStatus();

    void perform(Method method, std::function<void(const std::error_code&, Response response)> cb);
    void perform(Method method, const std::string& body, std::function<void(const std::error_code&, Response response)> cb);
    void perform(Method method, uint8_t* data, std::function<void(const std::error_code&, StatusCode, uint8_t* data)> cb);

    Future<Response> perform(Method method);
    Future<Response> perform(Method method, std::string_view body);
    Future<StatusCode> perform(Method method, uint8_t* data);

    void reset();

private:
    void checkTimeout(const asio_error_code& ec);

    void setMethodType(Method method);
    void performRequest(std::function<void(const std::error_code&)> cb);
    void receiveData(std::function<void(const std::error_code&)> cb);
    void receiveHeaderData(std::function<void(const std::error_code&)> cb);

    Future<void> performRequest();
    Future<void> receiveData();
    Future<void> receiveHeaderData();

    URI m_url;
    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    std::chrono::milliseconds m_timeout;
    beast::http::request<beast::http::string_body> m_request;
    beast::http::response<beast::http::string_body> m_response;
    beast::flat_buffer m_buffer;
};

}
}
