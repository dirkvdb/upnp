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

#include "upnp/upnphttpclient.h"

#include <upnp/upnp.h>
#include <cassert>
#include <stdexcept>
#include <sstream>

#include "utils/stringoperations.h"

using namespace utils;

namespace upnp
{

HttpClient::HttpClient(int32_t timeout)
: m_Timeout(timeout)
{
}

size_t HttpClient::getContentLength(const std::string& url)
{
	int32_t httpStatus = 0;
    int contentLength = 0;
	
    void* pHandle = open(url, contentLength, httpStatus);    
    UpnpCloseHttpGet(pHandle);
    
    throwOnBadHttpStatus(url, httpStatus);
    return contentLength;
}

std::string HttpClient::getText(const std::string& url)
{
    std::string data;
    int32_t httpStatus = 0;
    int contentLength = 0;
	
    void* pHandle = open(url, contentLength, httpStatus);
    data.resize(contentLength);
    read(pHandle, reinterpret_cast<uint8_t*>(&data.front()), contentLength);
    assert(data[data.size()] == '\0');
    
    UpnpCloseHttpGet(pHandle);
    
    throwOnBadHttpStatus(url, httpStatus);
    return data;
}

std::vector<uint8_t> HttpClient::getData(const std::string& url)
{
    std::vector<uint8_t> data;
    
    int32_t httpStatus = 0;
    int contentLength = 0;
    
    void* pHandle = open(url, contentLength, httpStatus);

	data.resize(contentLength);
    read(pHandle, data.data(), contentLength);
    UpnpCloseHttpGet(pHandle);
    
    throwOnBadHttpStatus(url, httpStatus);
    return data;
}

std::vector<uint8_t> HttpClient::getData(const std::string& url, uint64_t offset, uint64_t size)
{
    std::vector<uint8_t> data;
    
    int32_t httpStatus = 0;
    int contentLength = 0;
    
    void* pHandle = open(url, contentLength, httpStatus, offset, size);

	data.resize(contentLength);
    read(pHandle, data.data(), contentLength);
    UpnpCloseHttpGet(pHandle);
    
    throwOnBadHttpStatus(url, httpStatus);
    return data;
}

void HttpClient::getData(const std::string& url, uint8_t* pData)
{
    int32_t httpStatus = 0;
    int contentLength = 0;
    
    void* pHandle = open(url, contentLength, httpStatus);
    read(pHandle, pData, contentLength);
    UpnpCloseHttpGet(pHandle);
}

void HttpClient::getData(const std::string& url, uint8_t* pData, uint64_t offset, uint64_t size)
{
    int32_t httpStatus = 0;
    int contentLength = 0;
    
    void* pHandle = open(url, contentLength, httpStatus, offset, size);
    read(pHandle, pData, contentLength);
    UpnpCloseHttpGet(pHandle);
    
    throwOnBadHttpStatus(url, httpStatus);
}

void* HttpClient::open(const std::string& url, int32_t& contentLength, int32_t& httpStatus)
{
    void* pHandle       = nullptr;
    char* pContentType  = nullptr;

    auto ret = UpnpOpenHttpGet(url.c_str(), &pHandle, &pContentType, &contentLength, &httpStatus, m_Timeout);
	if (ret != UPNP_E_SUCCESS)
	{
		UpnpCloseHttpGet(pHandle);
		throw std::logic_error(stringops::format("Failed to get http url %s (%d)", url, ret));
	}
    
    return pHandle;
}

void* HttpClient::open(const std::string& url, int32_t& contentLength, int32_t& httpStatus, uint64_t offset, uint64_t size)
{
    void* pHandle       = nullptr;
    char* pContentType  = nullptr;

    auto ret = UpnpOpenHttpGetEx(url.c_str(), &pHandle, &pContentType, &contentLength, &httpStatus, static_cast<int32_t>(offset), static_cast<int32_t>(offset + size - 1), m_Timeout);
	if (ret != UPNP_E_SUCCESS)
	{
		UpnpCloseHttpGet(pHandle);
		throw std::logic_error(stringops::format("Failed to get http url %s (%d)", url, ret));
	}
    
    return pHandle;
}

void HttpClient::read(void* pHandle, uint8_t* pData, size_t dataSize)
{
    auto sizeCopy = dataSize;

    auto ret = UpnpReadHttpGet(pHandle, reinterpret_cast<char*>(pData), &sizeCopy, m_Timeout);
	if (ret != UPNP_E_SUCCESS)
	{
        UpnpCloseHttpGet(pHandle);
        throw std::logic_error(stringops::format("Failed to get url data (%d)", ret));
	}
    
    if (sizeCopy != dataSize)
    {
        throw std::logic_error(stringops::format("Incorrect bytes read from (%d <-> %d)", dataSize, sizeCopy));
    }
}

void HttpClient::throwOnBadHttpStatus(const std::string& url, int32_t status)
{
    // 206 is for partial content
    if (status != 200 && status != 206)
    {
        throw std::logic_error(stringops::format("Incorrect http status for %s (%d)", url, status));
    }
}

}
