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
#include <functional>

#include "upnp/upnp.device.h"

namespace upnp
{

struct SubscriptionRequest
{
    std::string serviceType;
    std::string sid;
    std::chrono::seconds timeout;
};

struct SubscriptionResponse
{
    // Actual timeout as decided by the service implementation
    std::chrono::seconds timeout;
    std::string initialEvent;
};

struct ActionRequest
{
    std::string serviceType;
    std::string actionName;
    std::string action;
};

class IRootDevice
{
public:
    virtual ~IRootDevice() = default;

    virtual void initialize() = 0;
    virtual void uninitialize() = 0;

    virtual std::string getWebrootUrl() = 0;
    virtual void registerDevice(const std::string& deviceDescriptionXml, const Device& dev) = 0;

    virtual std::string getUniqueDeviceName() = 0;

    virtual void notifyEvent(const std::string& serviceId, std::string response) = 0;

    virtual void addFileToHttpServer(const std::string& path, const std::string& contentType, const std::string& data) = 0;
    virtual void addFileToHttpServer(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data) = 0;
    virtual void removeFileFromHttpServer(const std::string& path) = 0;

    std::function<SubscriptionResponse(const SubscriptionRequest&)> EventSubscriptionRequested;
    std::function<std::string(const ActionRequest&)> ControlActionRequested;
};

}
