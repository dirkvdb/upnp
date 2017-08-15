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
#include "upnp.http.utils.h"
#include "upnp.enumutils.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include "utils/format.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.soap.parseutils.h"

#include "URI.h"
#include "upnp/stringview.h"

namespace upnp
{
namespace http
{

using namespace asio;
using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string s_delimiter = "\r\n";

namespace
{

std::error_code convertError(const asio_error_code& error)
{
    if (error.value() == asio::error::timed_out)
    {
        return std::make_error_code(error::Timeout);
    }

    log::error("Error performing Http call: {}", error.message());
    return std::make_error_code(error::NetworkError);
}

bool invokeCallbackOnError(const asio_error_code& error, const std::function<void(const std::error_code&)>& cb)
{
    if (!error)
    {
        return false;
    }

    cb(convertError(error));
    return true;
}

}

Client::Client(asio::io_service& io)
: m_timer(io)
, m_socket(io)
, m_timeout(60000ms)
{
    m_request.version = 11;
}

void Client::reset()
{
    m_buffer = beast::flat_buffer();
    m_request = beast::http::request<beast::http::string_body>();
}

void Client::setTimeout(std::chrono::milliseconds timeout) noexcept
{
    m_timeout = timeout;
}

void Client::addHeader(const std::string& name, const std::string& value)
{
    m_request.insert(name, value);
}

void Client::setUrl(const std::string& url)
{
    m_url = url;
}

std::string_view Client::getResponseHeaderValue(const char* headerValue)
{
    return sv_cast<std::string_view>(m_response[headerValue]);
}

std::string Client::getResponseBody()
{
    return m_response.body;
}

uint32_t Client::getStatus()
{
    return enum_value(m_response.result());
}

void Client::performRequest(std::function<void(const std::error_code&)> cb)
{
    asio_error_code error;
    m_socket.close(error);
    if (invokeCallbackOnError(error, cb)) return;

    auto addr = ip::tcp::endpoint(ip::address::from_string(m_url.getHost()), m_url.getPort());

    m_socket.open(addr.protocol(), error);
    if (invokeCallbackOnError(error, cb)) return;

    m_timer.expires_from_now(m_timeout);
    m_socket.async_connect(addr, [this, cb] (const asio_error_code& error) {
        if (!m_socket.is_open())
        {
            cb(std::make_error_code(error::Timeout));
            return;
        }

        m_timer.cancel();
        if (invokeCallbackOnError(error, cb))
        {
            return;
        }

        m_request.target(m_url.getPath());
        m_request.set("Host", fmt::format("{}:{}", m_url.getHost(), m_socket.remote_endpoint().port()));
        m_request.prepare_payload();
        beast::http::async_write(m_socket, m_request, [this, cb] (const beast::error_code& error) {
            if (invokeCallbackOnError(error, cb))
            {
                return;
            }

            if (m_request.method() == beast::http::verb::head)
            {
                receiveHeaderData(cb);
            }
            else
            {
                receiveData(cb);
            }
        });
    });

    m_timer.async_wait([this] (const asio_error_code& ec) { checkTimeout(ec); });
}

void Client::receiveData(std::function<void(const std::error_code&)> cb)
{
    m_timer.expires_from_now(m_timeout);

    beast::http::async_read(m_socket, m_buffer, m_response, [this, cb] (const asio_error_code& error) {
        m_timer.cancel();
        if (invokeCallbackOnError(error, cb))
        {
            return;
        }

        try
        {
            auto connValue = m_response["Connection"];
            if (strncasecmp(connValue.data(), "close", connValue.size()))
            {
                asio_error_code error;
                m_socket.close(error);
            }

            cb(std::error_code());
        }
        catch (const std::exception& e)
        {
            log::error("Failed to parse http response: {}", e.what());
            cb(std::make_error_code(error::InvalidResponse));
        }
    });
}

void Client::receiveHeaderData(std::function<void(const std::error_code&)> cb)
{
    m_timer.expires_from_now(m_timeout);

    auto parser = std::make_shared<beast::http::parser<false, beast::http::empty_body>>();
    beast::http::async_read_header(m_socket, m_buffer, *parser, [this, cb, parser] (const asio_error_code& error) {
        m_timer.cancel();
        if (invokeCallbackOnError(error, cb))
        {
            return;
        }

        try
        {
            log::info("read head: {}", parser->get().result());
            auto connValue = parser->get()["Connection"];
            log::info("read head: {}", connValue);
            if (strncasecmp(connValue.data(), "close", connValue.size()))
            {
                asio_error_code error;
                m_socket.close(error);
            }

            for (auto& field : parser->get().base())
            {
                m_response.insert(field.name(), field.value());
            }
            cb(std::error_code());
        }
        catch (const std::exception& e)
        {
            log::error("Failed to parse http response: {}", e.what());
            cb(std::make_error_code(error::InvalidResponse));
        }
    });
}

static beast::http::verb methodToVerb(Method m)
{
    switch (m)
    {
    case Method::Head:          return beast::http::verb::head;
    case Method::Notify:        return beast::http::verb::notify;
    case Method::Search:        return beast::http::verb::msearch;
    case Method::Subscribe:     return beast::http::verb::subscribe;
    case Method::Unsubscribe:   return beast::http::verb::unsubscribe;
    case Method::Get:           return beast::http::verb::get;
    case Method::Post:          return beast::http::verb::post;
    default:
        throw std::invalid_argument("Invalid http method provided");
    }
}

void Client::setMethodType(Method method)
{
    m_request.method(methodToVerb(method));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
}

void Client::perform(Method method, std::function<void(const std::error_code&, Response)> cb)
{
    setMethodType(method);
    performRequest([this, cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, Response());
        }
        else
        {
            cb(error, Response(StatusCode(m_response.result()), m_response.body));
        }
    });
}

void Client::perform(Method method, const std::string& body, std::function<void(const std::error_code&, Response)> cb)
{
    setMethodType(method);
    m_request.body = body;
    performRequest([this, cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, Response());
        }
        else
        {
            cb(error, Response(StatusCode(m_response.result()), m_response.body));
        }
    });
}

void Client::perform(Method method, uint8_t* data, std::function<void(const std::error_code&, StatusCode, uint8_t*)> cb)
{
    setMethodType(method);

    performRequest([this, cb, data] (const std::error_code& error) {
        if (error)
        {
            cb(error, StatusCode::None, nullptr);
        }

        memcpy(data, m_response.body.data(), m_response.body.size());
        cb(std::error_code(), StatusCode(m_response.result()), data);
    });
}

void Client::checkTimeout(const asio_error_code& ec)
{
    if (ec.value() == asio::error::operation_aborted)
    {
        return;
    }

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (m_timer.expires_at() <= std::chrono::steady_clock::now())
    {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        asio_error_code error;
        m_socket.close(error);

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        m_timer.expires_at(std::chrono::steady_clock::time_point::max());
        return;
    }

    m_timer.async_wait([this] (const asio_error_code& ec) { checkTimeout(ec); });
}

}
}
