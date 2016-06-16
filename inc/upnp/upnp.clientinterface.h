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

#include <chrono>
#include <vector>
#include <cinttypes>
#include <functional>

#include "upnp/upnp.types.h"

namespace asio { class io_service; }

namespace upnp
{

namespace uv { class Loop; }

class Action;

class IClient
{
public:
    virtual ~IClient() = default;

    virtual void initialize() = 0;
    virtual void initialize(const std::string& interfaceName, uint16_t port) = 0;
    virtual void uninitialize() = 0;

    virtual std::string getIpAddress() const = 0;
    virtual uint16_t getPort() const = 0;

    virtual void subscribeToService(const std::string& publisherUrl, std::chrono::seconds timeout, std::function<std::function<void(SubscriptionEvent)>(Status status, std::string subId, std::chrono::seconds timeout)> cb) = 0;
    virtual void renewSubscription(const std::string& publisherUrl, const std::string& subscriptionId, std::chrono::seconds timeout, std::function<void(Status status, std::string subId, std::chrono::seconds timeout)> cb) = 0;
    virtual void unsubscribeFromService(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(Status status)> cb) = 0;

    virtual void sendAction(const Action& action, std::function<void(Status, std::string actionResult)> cb) = 0;
    virtual void getFile(const std::string& url, std::function<void(Status, std::string contents)> cb) = 0;

    virtual uv::Loop& loop() noexcept = 0;
    virtual asio::io_service& ioService() noexcept = 0;
};

}
