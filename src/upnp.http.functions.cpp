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

#include "upnp/upnp.http.functions.h"

#include "upnp/asio.h"
#include "upnp.http.client.h"

#include "utils/format.h"

namespace upnp
{
namespace http
{

using namespace std::chrono_literals;

static const auto s_defaultTimeout = 30s;

namespace
{

std::string createRangeHeaderValue(size_t offset, size_t size)
{
    if (size == 0)
    {
        return fmt::format("bytes={}-", offset);
    }

    return fmt::format("bytes={}-{}", offset, offset + size - 1);
}

}

void get(asio::io_service& io, const std::string& url, std::function<void(const std::error_code& error, Response)> cb)
{
    get(io, url, s_defaultTimeout, cb);
}

void get(asio::io_service& io, const std::string& url, std::chrono::seconds timeout, std::function<void(const std::error_code& error, Response)> cb)
{
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    client->perform(Method::Get, [client, cb] (const std::error_code& error, Response response) {
        cb(error, std::move(response));
    });
}

void get(asio::io_service& io, const std::string& url, uint8_t* data, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb)
{
    get(io, url, data, s_defaultTimeout, cb);
}

void get(asio::io_service& io, const std::string& url, uint8_t* data, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb)
{
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    client->perform(Method::Get, data, [client, cb] (const std::error_code& error, StatusCode status, uint8_t* data) {
        cb(error, status, data);
    });
}

void getRange(asio::io_service& io, const std::string& url, uint32_t offset, uint32_t size, std::function<void(const std::error_code& error, Response)> cb)
{
    getRange(io, url, offset, size, s_defaultTimeout, cb);
}

void getRange(asio::io_service& io, const std::string& url, uint32_t offset, uint32_t size, std::chrono::seconds timeout, std::function<void(const std::error_code& error, Response)> cb)
{
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    client->addHeader("Range", createRangeHeaderValue(offset, size));
    client->perform(Method::Get, [client, cb] (const std::error_code& error, Response response) {
        cb(error, std::move(response));
    });
}

void getRange(asio::io_service& io, const std::string& url, uint32_t offset, uint32_t size, uint8_t* data, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb)
{
    getRange(io, url, offset, size, data, s_defaultTimeout, cb);
}

void getRange(asio::io_service& io, const std::string& url, uint32_t offset, uint32_t size, uint8_t* data, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb)
{
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    client->addHeader("Range", createRangeHeaderValue(offset, size));
    client->perform(Method::Get, data, [client, cb] (const std::error_code& error, StatusCode status, uint8_t* dataPtr) {
        cb(error, status, dataPtr);
    });
}

void getContentLength(asio::io_service& io, const std::string& url, std::function<void(const std::error_code& error, StatusCode, size_t)> cb)
{
    getContentLength(io, url, s_defaultTimeout, cb);
}

void getContentLength(asio::io_service& io, const std::string& url, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, size_t)> cb)
{
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    client->perform(Method::Head, [client, cb] (const std::error_code& error, const Response& response) {
        try
        {
            cb(error, response.status, response.status == StatusCode::Ok ? std::stoul(std::string(client->getResponseHeaderValue("Content-length"))) : 0);
        }
        catch (const std::exception& e)
        {
            cb(std::make_error_code(error::InvalidResponse), response.status, 0);
        }
    });
}

Task<std::tuple<StatusCode, size_t>> getContentLength(asio::io_service& io, const std::string& url)
{
    utils::log::debug("getcontent length");
    return getContentLength(io, url, s_defaultTimeout);
}

Task<std::tuple<StatusCode, size_t>> getContentLength(asio::io_service& io, const std::string& url, std::chrono::seconds timeout)
{
    assert(false);
    utils::log::debug("getcontent length timeout");
    auto client = std::make_shared<http::Client>(io);
    client->setUrl(url);
    client->setTimeout(timeout);
    utils::log::debug("getcontent length perform");
    auto response = co_await client->perform(Method::Head);
    size_t length = 0;
    if (response.status == StatusCode::Ok)
    {
        length = std::stoul(std::string(client->getResponseHeaderValue("Content-length")));
    }

    co_return std::make_tuple(response.status, length);
}

}
}
