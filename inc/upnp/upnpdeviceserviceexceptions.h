//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#ifndef UPNP_DEVICE_SERVICE_EXPCEPTIONS_H
#define UPNP_DEVICE_SERVICE_EXPCEPTIONS_H

#include <string>
#include <map>
#include <chrono>

#include "utils/types.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpactionresponse.h"

namespace upnp
{

class ServiceException : public std::runtime_error
{
public:
    ServiceException(const std::string& msg, uint32_t errorCode)
    : std::runtime_error(msg)
    , m_ErrorCode(errorCode)
    {
    }
    
    uint32_t errorCode()
    {
        return m_ErrorCode;
    }

private:
    uint32_t    m_ErrorCode;
};

class InvalidActionException : public ServiceException
{
public:
    InvalidActionException() : ServiceException("Invalid action", 401) {}
};

class InvalidArgumentsException : public ServiceException
{
public:
    InvalidArgumentsException() : ServiceException("Invalid arguments", 402) {}
};

class ActionFailedException : public ServiceException
{
public:
    ActionFailedException() : ServiceException("Action failed", 501) {}
};

class InvalidNameException : public ServiceException
{
public:
    InvalidNameException() : ServiceException("Invalid name", 701) {}
};

class InvalidInstanceIdException : public ServiceException
{
public:
    InvalidInstanceIdException() : ServiceException("Invalid InstanceID", 702) {}
};

}

#endif
