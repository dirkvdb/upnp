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

#include <stdexcept>

using namespace utils;

namespace upnp
{

HttpReader::HttpReader()
: m_httpClient(5)
, m_ContentLength(0)
, m_CurrentPosition(0)
{
}

void HttpReader::open(const std::string& url)
{
    m_Url = url;
    m_ContentLength = m_httpClient.getContentLength(url);
}

void HttpReader::close()
{
}

uint64_t HttpReader::getContentLength()
{
    throwOnEmptyUrl();

    return m_ContentLength;
}

uint64_t HttpReader::currentPosition()
{
    throwOnEmptyUrl();

    return m_CurrentPosition;
}

void HttpReader::seekAbsolute(uint64_t position)
{
    throwOnEmptyUrl();

    m_CurrentPosition = position;
}

void HttpReader::seekRelative(uint64_t offset)
{
    throwOnEmptyUrl();
    
    m_CurrentPosition += offset;
}

bool HttpReader::eof()
{
    throwOnEmptyUrl();

    return m_CurrentPosition >= m_ContentLength;
}

std::string HttpReader::uri()
{
    return m_Url;
}

uint64_t HttpReader::read(uint8_t* pData, uint64_t size)
{
    throwOnEmptyUrl();
    
    uint64_t upperLimit = m_CurrentPosition + size;
    if (upperLimit >= m_ContentLength)
    {
        upperLimit = m_ContentLength;
        size = eof() ? 0 : m_ContentLength - m_CurrentPosition;
    }
    
    if (size > 0) // avoid requests when eof reached
    {
        m_httpClient.getData(m_Url, pData, m_CurrentPosition, upperLimit - m_CurrentPosition);
        m_CurrentPosition += size;
        m_CurrentPosition = std::min(m_ContentLength, m_CurrentPosition);
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
        throw std::runtime_error("Failed to read all file data for url: " + m_Url);
    }
    
    return data;
}

void HttpReader::clearErrors()
{
}

void HttpReader::throwOnEmptyUrl()
{
    if (m_Url.empty())
    {
        throw std::logic_error("HttpReader: no url was opened yet");
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
        throw std::logic_error("Uri is not supported by Http reader: " + uri);
    }
    
    return new upnp::HttpReader();
}

}
