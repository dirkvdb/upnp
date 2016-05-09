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

#include "upnp/upnphttpreader.h"
#include "utils/log.h"
#include "upnp/upnptypes.h"
#include "utils/format.h"

#include <curl/curl.h>
#include <stdexcept>

using namespace utils;

namespace upnp
{

static const long s_timeout = 10;

class CurlHandle
{
public:
    CurlHandle()
    : m_curl(curl_easy_init())
    {
        if (!m_curl)
        {
            throw std::runtime_error("Failed to initialize curl handle");
        }
    }
    
    ~CurlHandle()
    {
        curl_easy_cleanup(m_curl);
    }
    
    operator CURL*()
    {
        return m_curl;
    }
    
private:
    CURL* m_curl;
};

HttpReader::HttpReader()
: m_contentLength(0)
, m_currentPosition(0)
{
}

void HttpReader::open(const std::string& url)
{
    m_url = url;
    
    CurlHandle curl;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, s_timeout);
    
    auto res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        throw std::runtime_error("Failed to open url: " + std::string(curl_easy_strerror(res)));
    }
    
    double contentLength;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
    m_contentLength = static_cast<uint64_t>(contentLength);
}

void HttpReader::close()
{
}

uint64_t HttpReader::getContentLength()
{
    throwOnEmptyUrl();

    return m_contentLength;
}

uint64_t HttpReader::currentPosition()
{
    throwOnEmptyUrl();

    return m_currentPosition;
}

void HttpReader::seekAbsolute(uint64_t position)
{
    throwOnEmptyUrl();

    m_currentPosition = position;
}

void HttpReader::seekRelative(uint64_t offset)
{
    throwOnEmptyUrl();
    
    m_currentPosition += offset;
}

bool HttpReader::eof()
{
    throwOnEmptyUrl();

    return m_currentPosition >= m_contentLength;
}

std::string HttpReader::uri()
{
    return m_url;
}

struct WriteData
{
    uint8_t* ptr = nullptr;
    size_t offset = 0;
};

static size_t writeFunc(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* writeData = reinterpret_cast<WriteData*>(userdata);
    auto dataSize = size * nmemb;
    memcpy(writeData->ptr + writeData->offset, ptr, dataSize);
    writeData->offset += dataSize;
    return dataSize;
}

uint64_t HttpReader::read(uint8_t* pData, uint64_t size)
{
    throwOnEmptyUrl();
    
    uint64_t upperLimit = m_currentPosition + size - 1;
    if (upperLimit >= m_contentLength)
    {
        upperLimit = m_contentLength - 1;
        size = eof() ? 0 : m_contentLength - m_currentPosition;
    }
    
    if (size > 0) // avoid requests when eof reached
    {
        auto range = fmt::format("{}-{}", m_currentPosition, upperLimit);
        WriteData writeData{ pData, 0 };
        
        CurlHandle curl;
        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, s_timeout);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
        
        auto res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            throw std::runtime_error("Failed to open url: " + std::string(curl_easy_strerror(res)));
        }
    
        m_currentPosition += size;
        m_currentPosition = std::min(m_contentLength, m_currentPosition);
    }
    
    return size;
}

std::vector<uint8_t> HttpReader::readAllData()
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

void HttpReader::clearErrors()
{
}

void HttpReader::throwOnEmptyUrl()
{
    if (m_url.empty())
    {
        throw Exception("HttpReader: no url was opened yet");
    }
}

bool HttpReaderBuilder::supportsUri(const std::string& uri)
{
    static const std::string http("http://");

    return uri.compare(0, http.length(), http) == 0;
}

utils::IReader* HttpReaderBuilder::build(const std::string& uri)
{
    if (!supportsUri(uri))
    {
        throw Exception("Uri is not supported by Http reader: " + uri);
    }
    
    return new upnp::HttpReader();
}

}
