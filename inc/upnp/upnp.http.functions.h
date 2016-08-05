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
#include <chrono>
#include <functional>
#include <system_error>

#include "upnp/upnp.http.types.h"

namespace asio
{
    class io_service;
}

namespace upnp
{
namespace http
{

void get(asio::io_service& io, const std::string& url, std::function<void(const std::error_code& error, Response)> cb);
void get(asio::io_service& io, const std::string& url, std::chrono::seconds timeout, std::function<void(const std::error_code& error, Response)> cb);
void get(asio::io_service& io, const std::string& url, uint8_t* data, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb);
void get(asio::io_service& io, const std::string& url, uint8_t* data, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb);

void getRange(asio::io_service& io, const std::string& url, uint32_t start, uint32_t end, std::function<void(const std::error_code& error, Response)> cb);
void getRange(asio::io_service& io, const std::string& url, uint32_t start, uint32_t end, std::chrono::seconds timeout, std::function<void(const std::error_code& error, Response)> cb);
void getRange(asio::io_service& io, const std::string& url, uint32_t start, uint32_t end, uint8_t* data, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb);
void getRange(asio::io_service& io, const std::string& url, uint32_t start, uint32_t end, uint8_t* data, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, uint8_t*)> cb);

void getContentLength(asio::io_service& io, const std::string& url, std::function<void(const std::error_code& error, StatusCode, size_t)> cb);
void getContentLength(asio::io_service& io, const std::string& url, std::chrono::seconds timeout, std::function<void(const std::error_code& error, StatusCode, size_t)> cb);

}
}
