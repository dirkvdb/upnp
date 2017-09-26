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

#include <thread>
#include <unordered_map>

#include "upnp/asio.h"
#include "upnp/upnp.clientinterface.h"

namespace upnp
{

namespace gena { class Server; }
namespace soap { class Client; }

class Client : public IClient
{
public:
    Client();
    Client(asio::io_service& io);
    virtual ~Client();

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

    void sendAction(const Action& action, std::function<void(Status, soap::ActionResult actionResult)> cb) override;
    void getFile(const std::string& url, std::function<void(Status, std::string contents)> cb) override;

    Future<soap::ActionResult> sendAction(const Action& action) override;
    Future<std::string> getFile(const std::string& url) override;

    void dispatch(std::function<void()> func) override;
    asio::io_service& ioService() noexcept override;

private:
    void initialize(const asio::ip::tcp::endpoint& address);

    void handlEvent(const SubscriptionEvent& event);

    std::unique_ptr<std::thread> m_asioThread;
    std::unique_ptr<asio::io_service> m_owningIo;
    asio::io_service& m_io;
    std::unique_ptr<gena::Server> m_eventServer;
    std::unordered_map<std::string, std::function<void(SubscriptionEvent)>> m_eventCallbacks;
};

}
