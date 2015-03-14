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

#ifndef UPNP_HTTP_CLIENT_H
#define UPNP_HTTP_CLIENT_H

#include <string>
#include <vector>
#include <cinttypes>

namespace upnp
{

class HttpClient
{
public:
	HttpClient(int32_t commandTimeout);
    HttpClient(const HttpClient&) = delete;

    size_t getContentLength(const std::string& url);
    std::string getText(const std::string& url);

    std::vector<uint8_t> getData(const std::string& url);
    std::vector<uint8_t> getData(const std::string& url, uint64_t offset, uint64_t size);

    void getData(const std::string& url, uint8_t* pData);
    void getData(const std::string& url, uint8_t* pData, uint64_t offset, uint64_t size);

private:
    void* open(const std::string& url, int& contentLength, int32_t& httpStatus);
    void* open(const std::string& url, int32_t& contentLength, int32_t& httpStatus, uint64_t offset, uint64_t size);
    void read(void* pHandle, uint8_t* pData, size_t dataSize);
    void throwOnBadHttpStatus(const std::string& url, int32_t status);

	int32_t		m_Timeout;
};

}

#endif
