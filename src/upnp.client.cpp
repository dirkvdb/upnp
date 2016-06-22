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

#include "upnp.client.h"

#include "utils/log.h"
#include "upnp/upnp.asio.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.client.h"
#include "upnp.gena.server.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

using namespace asio;
using namespace utils;
using namespace std::placeholders;

namespace
{

Status httpStatusToStatus(const std::error_code& error)
{
    if (error.value() < 0)
    {
        return Status(ErrorCode::NetworkError, error.message());
    }
    else if (error.value() != 200)
    {
        return Status(ErrorCode::HttpError, error.message());
    }

    return Status();
}

}

Client::Client()
: m_io(std::make_unique<asio::io_service>())
{
}

Client::~Client() = default;

void Client::initialize()
{
    log::debug("Initializing UPnP SDK");
    return initialize(ip::tcp::endpoint(ip::address::address(), 0));
}

void Client::initialize(const std::string& interfaceName, uint16_t port)
{
    auto addr = getNetworkInterfaceV4(interfaceName).address;
    initialize(ip::tcp::endpoint(addr, port));
}

void Client::initialize(const asio::ip::tcp::endpoint& addr)
{
    m_httpClient = std::make_unique<http::Client>(*m_io);
    m_eventServer = std::make_unique<gena::Server>(*m_io, addr, [&] (const SubscriptionEvent& ev) {
        auto iter = m_eventCallbacks.find(ev.sid);
        if (iter != m_eventCallbacks.end())
        {
            iter->second(ev);
        }
    });

    m_asioThread = std::make_unique<std::thread>([this] () {
        asio::io_service::work work(*m_io);
        m_io->run();
    });
}

void Client::uninitialize()
{
    log::debug("Uninitializing UPnP SDK");

    m_eventServer->stop();
    m_io->stop();

    m_httpClient.reset();
    m_eventServer.reset();

    m_asioThread->join();
    m_asioThread.reset();
}

std::string Client::getIpAddress() const
{
    return m_eventServer->getAddress().address().to_string();
}

uint16_t Client::getPort() const
{
    return m_eventServer->getAddress().port();
}

void Client::subscribeToService(const std::string& publisherUrl, std::chrono::seconds timeout, std::function<std::function<void(SubscriptionEvent)>(Status, std::string, std::chrono::seconds)> cb)
{
    if (!m_eventServer)
    {
        throw std::runtime_error("UPnP library is not properly initialized");
    }

    auto addr = m_eventServer->getAddress();
    auto eventServerUrl = fmt::format("http://{}:{}/", addr.address(), addr.port());

    m_io->post([=] () {
#ifdef DEBUG_UPNP_CLIENT
        log::debug("Subscribe to service: {}", publisherUrl);
#endif
        m_httpClient->subscribe(publisherUrl, eventServerUrl, timeout, [this, cb] (const std::error_code& error, std::string subId, std::chrono::seconds subTimeout, std::string response) {
            //log::debug("Subscribe response: {}", response);

            if (cb)
            {
                auto subCb = cb(httpStatusToStatus(error), subId, subTimeout);
                if (subCb)
                {
                    m_eventCallbacks.emplace(subId, subCb);
                }
            }
        });
    });
}

void Client::renewSubscription(const std::string& publisherUrl,
                               const std::string& subscriptionId,
                               std::chrono::seconds timeout,
                               std::function<void(Status status, std::string subId, std::chrono::seconds timeout)> cb)
{
    assert(m_eventServer);
    assert(timeout.count() > 0);

    m_io->post([=] () {
#ifdef DEBUG_UPNP_CLIENT
        log::debug("Renew subscription: {} {}", publisherUrl, subscriptionId);
#endif
        m_httpClient->renewSubscription(publisherUrl, subscriptionId, timeout, [this, cb] (const std::error_code& error, std::string subId, std::chrono::seconds subTimeout, std::string response) {
            //log::debug("Subscription renewal response: {}", response);

            if (cb)
            {
                cb(httpStatusToStatus(error), subId, subTimeout);
            }
        });
    });
}

void Client::unsubscribeFromService(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(Status status)> cb)
{
    m_io->post([=] () {
        m_httpClient->unsubscribe(publisherUrl, subscriptionId, [=] (const std::error_code& error, std::string response) {
//#ifdef DEBUG_UPNP_CLIENT
            log::debug("Unsubscribe response: {}", response);
//#endif
            if (cb)
            {
                cb(httpStatusToStatus(error));
            }

            m_eventCallbacks.erase(subscriptionId);
        });
    });
}

void Client::sendAction(const Action& action, std::function<void(Status, std::string)> cb)
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    auto env = std::make_shared<std::string>(action.toString());
    m_io->post([this, url = action.getUrl(), name = action.getName(), urn = action.getServiceTypeUrn(), env, cb = std::move(cb)] () {
        m_httpClient->soapAction(url, name, urn, *env, [cb, env] (const std::error_code& error, std::string response) {
            cb(httpStatusToStatus(error), std::move(response));
        });
    });

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
}

void Client::getFile(const std::string& url, std::function<void(Status, std::string contents)> cb)
{
    m_httpClient->get(url, [cb] (const std::error_code& error, std::string contents) {
        cb(httpStatusToStatus(error), std::move(contents));
    });
}

asio::io_service& Client::ioService() noexcept
{
    return *m_io;
}

void Client::handlEvent(const SubscriptionEvent& event)
{
    auto iter = m_eventCallbacks.find(event.sid);
    if (iter != m_eventCallbacks.end())
    {
        iter->second(event);
    }
}

}
