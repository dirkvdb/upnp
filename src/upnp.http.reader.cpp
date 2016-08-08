//    Copyright (C) 2009 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include "upnp/upnp.http.reader.h"
#include "upnp/upnp.http.functions.h"
#include "upnp.http.client.h"
#include "upnp/upnptypes.h"
#include "utils/format.h"
#include "utils/log.h"

#include <asio.hpp>

#include <future>
#include <stdexcept>

namespace upnp
{
namespace http
{

using namespace utils;
using namespace std::chrono_literals;
using namespace std::string_literals;

static const auto s_timeout = 10s;

Reader::Reader()
: m_contentLength(0)
, m_currentPosition(0)
{
}

void Reader::open(const std::string& url)
{
    m_url = url;

    asio::io_service io;

    std::promise<size_t> prom;
    auto fut = prom.get_future();

    http::getContentLength(io, url, s_timeout, [&prom] (const std::error_code& error, StatusCode status, size_t contentLength) {
        if (error)
        {
            prom.set_exception(std::make_exception_ptr(Exception("Failed to open url: {}", error.message())));
        }
        else if (status != StatusCode::Ok)
        {
            prom.set_exception(std::make_exception_ptr(Exception("Failed to open url, HTTP error: {}", status_message(status))));
        }
        else
        {
            prom.set_value(contentLength);
        }
    });

    io.run();
    m_contentLength = fut.get();
}

void Reader::close()
{
}

uint64_t Reader::getContentLength()
{
    throwOnEmptyUrl();
    return m_contentLength;
}

uint64_t Reader::currentPosition()
{
    throwOnEmptyUrl();
    return m_currentPosition;
}

void Reader::seekAbsolute(uint64_t position)
{
    throwOnEmptyUrl();
    m_currentPosition = position;
}

void Reader::seekRelative(uint64_t offset)
{
    throwOnEmptyUrl();
    m_currentPosition += offset;
}

bool Reader::eof()
{
    throwOnEmptyUrl();
    return m_currentPosition >= m_contentLength;
}

std::string Reader::uri()
{
    return m_url;
}

uint64_t Reader::read(uint8_t* pData, uint64_t size)
{
    throwOnEmptyUrl();

    uint64_t upperLimit = m_currentPosition + size - 1;
    if (upperLimit >= m_contentLength)
    {
        upperLimit = m_contentLength - 1;
        size = eof() ? 0 : m_contentLength - m_currentPosition;
    }

    if (size == 0) // avoid requests when eof reached
    {
        return size;
    }

    asio::io_service io;

    std::promise<void> prom;
    auto fut = prom.get_future();

    http::getRange(io, m_url, m_currentPosition, size, pData, s_timeout, [&prom] (const std::error_code& error, StatusCode status, uint8_t*) {
        if (error)
        {
            prom.set_exception(std::make_exception_ptr(Exception("Failed to open url: {}", error.message())));
        }
        else if (status != StatusCode::PartialContent)
        {
            prom.set_exception(std::make_exception_ptr(Exception("Failed to open url, HTTP error: {}", status_message(status))));
        }
        else
        {
            prom.set_value();
        }
    });

    io.run();
    fut.get();

    m_currentPosition += size;
    m_currentPosition = std::min(m_contentLength, m_currentPosition);

    return size;
}

std::vector<uint8_t> Reader::readAllData()
{
    std::vector<uint8_t> data;
    data.resize(getContentLength());

    seekAbsolute(0);
    if (data.size() != read(data.data(), data.size()))
    {
        throw Exception("Failed to read all file data for url: {}", m_url);
    }

    return data;
}

void Reader::clearErrors()
{
}

void Reader::throwOnEmptyUrl()
{
    if (m_url.empty())
    {
        throw Exception("HttpReader: no url was opened yet");
    }
}

bool ReaderBuilder::supportsUri(const std::string& uri)
{
    static const std::string http("http://");
    return uri.compare(0, http.length(), http) == 0;
}

utils::IReader* ReaderBuilder::build(const std::string& uri)
{
    if (!supportsUri(uri))
    {
        throw Exception("Uri is not supported by Http reader: {}", uri);
    }

    return new Reader();
}

}
}
