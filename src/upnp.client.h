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

#include <unordered_map>

#include "upnp/upnp.clientinterface.h"
#include "upnp/upnp.types.h"
#include "upnp/upnp.http.client.h"

namespace uv { class Loop; }

namespace upnp
{

namespace gena { class Server; }

class Client2 : public IClient2
{
public:
    Client2();
    virtual ~Client2();

    void initialize() override;
    void initialize(const std::string& interfaceName, uint16_t port) override;
    void uninitialize() override;

    std::string getIpAddress() const override;
    uint16_t getPort() const override;

    void subscribeToService(const std::string& publisherUrl,
                            std::chrono::seconds timeout,
                            std::function<std::function<void(SubscriptionEvent)>(Status status, std::string subId, std::chrono::seconds timeout)> cb) override;

    void renewSubscription(const std::string& publisherUrl,
                           const std::string& subscriptionId,
                           std::chrono::seconds timeout,
                           std::function<void(Status status, std::string subId, std::chrono::seconds timeout)> cb) override;

    void unsubscribeFromService(const std::string& publisherUrl,
                                const std::string& subscriptionId,
                                std::function<void(Status status)> cb) override;

    void sendAction(const Action& action, std::function<void(Status, std::string actionResult)> cb) override;
    void getFile(const std::string& url, std::function<void(Status, std::string contents)> cb) override;

    uv::Loop& loop() const override;

private:
    void initialize(const uv::Address& addr);

    void handlEvent(const SubscriptionEvent& event);

    std::unique_ptr<std::thread> m_thread;
    std::unique_ptr<uv::Loop> m_loop;
    http::Client m_httpClient;
    std::unique_ptr<gena::Server> m_eventServer;
    std::unordered_map<std::string, std::function<void(SubscriptionEvent)>> m_eventCallbacks;
};

}
