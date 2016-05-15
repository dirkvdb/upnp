

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

#include <map>
#include <string>
#include <memory>
#include <stdexcept>

#include "utils/format.h"
#include "upnp/upnpfwd.h"
#include "upnp/upnp.types.h"
#include "upnp/upnp.protocolinfo.h"

namespace upnp
{

class Exception : public std::runtime_error
{
public:
    Exception(const char* msg) : std::runtime_error(msg), m_errorCode(0) {}
    Exception(const std::string& msg) : Exception(msg.c_str()) {}

    template<typename... T>
    Exception(const char* fmt, const T&... args) : Exception(0, fmt, std::forward<const T>(args)...) {}

    Exception(int32_t errorCode, const char* msg) : std::runtime_error(msg), m_errorCode(errorCode) {}
    Exception(int32_t errorCode, const std::string& msg) : Exception(errorCode, msg.c_str()) {}

    Exception(ErrorCode errorCode) : Exception(static_cast<int32_t>(errorCode), errorCodeToString(errorCode)) {}

    template<typename... T>
    Exception(int32_t errorCode, const char* fmt, T&&... args) : Exception(errorCode, fmt::format(fmt, std::forward<T&&>(args)...)) {}

    int32_t getErrorCode() { return m_errorCode; }

private:
    int32_t     m_errorCode;
};

}
