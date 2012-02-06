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

#ifndef UPNP_HTTP_GET_H
#define UPNP_HTTP_GET_H

#include <string>
#include <vector>

#include "utils/types.h"

namespace UPnP
{

class UPnPHttpGet
{
public:
	UPnPHttpGet(const std::string& url, int32_t timeout);
	UPnPHttpGet(const std::string& url, uint32_t offset, uint32_t size, int32_t timeout);
	~UPnPHttpGet();

	int32_t getContentLength();
	void get(std::vector<uint8_t>& data);
	void get(uint8_t* pData);

private:
	void* 		m_pHandle;
	int32_t		m_ContentLength;
	int32_t		m_Timeout;
};

}

#endif
