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

#include "upnp/upnphttpget.h"

#include <upnp/upnp.h>
#include <cassert>
#include <stdexcept>
#include <sstream>

namespace upnp
{

HttpGet::HttpGet(const std::string& url, int32_t timeout)
: m_pHandle(nullptr)
, m_ContentLength(0)
, m_Timeout(timeout)
{
	char* pContentType = nullptr;
	int32_t httpStatus = 0;

	int32_t ret = UpnpOpenHttpGet(url.c_str(), &m_pHandle, &pContentType, &m_ContentLength, &httpStatus, m_Timeout);
	if (ret != UPNP_E_SUCCESS)
	{
		UpnpCloseHttpGet(m_pHandle);
		std::stringstream ss;
		ss << "Failed to get http url " << url << "(" << ret << ")";
		throw std::logic_error(ss.str());
	}
}

HttpGet::HttpGet(const std::string& url, uint32_t offset, uint32_t size, int32_t timeout)
: m_pHandle(nullptr)
, m_ContentLength(0)
, m_Timeout(timeout)
{
	char* pContentType = nullptr;
	int32_t httpStatus = 0;

	int32_t ret = UpnpOpenHttpGetEx(url.c_str(), &m_pHandle, &pContentType, &m_ContentLength, &httpStatus, static_cast<int32_t>(offset), static_cast<int32_t>(offset + size), timeout);
	if (ret != UPNP_E_SUCCESS)
	{
		UpnpCloseHttpGet(m_pHandle);
		throw std::logic_error("Failed to open http connection to get content length: " + url);
	}
}

HttpGet::~HttpGet()
{
	if (m_pHandle)
	{
		UpnpCloseHttpGet(m_pHandle);
	}
}

int32_t HttpGet::getContentLength()
{
	return m_ContentLength;
}

void HttpGet::get(std::vector<uint8_t>& data)
{
	assert(m_pHandle);

	data.resize(m_ContentLength);
	get(&(data[0]));
}

void HttpGet::get(uint8_t* pData)
{
	assert(m_pHandle);

	size_t length = static_cast<size_t>(m_ContentLength);

	int32_t ret = UpnpReadHttpGet(m_pHandle, reinterpret_cast<char*>(pData), &length, m_Timeout);
	if (ret != UPNP_E_SUCCESS)
	{
		std::stringstream ss;
		ss << "Failed to get url data (" << ret << ")";
		throw std::logic_error(ss.str());
	}
}

}
