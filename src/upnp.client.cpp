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
#include "upnp/upnp.http.functions.h"
#include "upnp.http.client.h"
#include "upnp.soap.client.h"
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

Status httpStatusToStatus(const std::error_code& error, http::StatusCode status)
{
    if (error)
    {
        return Status(ErrorCode::NetworkError, error.message());
    }
    else if (status != http::StatusCode::Ok)
    {
        return Status(ErrorCode::HttpError, error.value(), error.message());
    }

    return Status(ErrorCode::Success, error.value());
}

}

Client::Client()
: m_owningIo(std::make_unique<asio::io_service>())
, m_io(m_owningIo.get())
{
}

Client::Client(asio::io_service& io)
: m_io(&io)
{
}

Client::~Client()
{
    if (m_asioThread)
    {
        uninitialize();
    }
}

void Client::initialize()
{
    return initialize(ip::tcp::endpoint(ip::address_v4::any(), 0));
}

void Client::initialize(const std::string& interfaceName, uint16_t port)
{
    auto addr = getNetworkInterfaceV4(interfaceName).address;
    initialize(ip::tcp::endpoint(addr, port));
}

void Client::initialize(const asio::ip::tcp::endpoint& addr)
{
    log::debug("Initializing UPnP SDK");
    m_eventServer = std::make_unique<gena::Server>(*m_io, addr, [&] (const SubscriptionEvent& ev) {
        auto iter = m_eventCallbacks.find(ev.sid);
        if (iter != m_eventCallbacks.end())
        {
            iter->second(ev);
        }
    });

    if (m_owningIo)
    {
        m_asioThread = std::make_unique<std::thread>([this] () {
            m_io->reset();
            asio::io_service::work work(*m_io);
            m_io->run();
        });
    }
}

void Client::uninitialize()
{
    log::debug("Uninitializing UPnP SDK");

    m_eventServer->stop();
    m_io->stop();

    m_eventServer.reset();

    if (m_asioThread)
    {
        m_asioThread->join();
        m_asioThread.reset();
    }
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
        auto soap = std::make_shared<soap::Client>(*m_io);
        soap->subscribe(publisherUrl, eventServerUrl, timeout, [this, cb, soap] (const std::error_code& error, http::StatusCode status, std::string subId, std::chrono::seconds subTimeout) {
            if (cb)
            {
                auto subCb = cb(httpStatusToStatus(error, status), subId, subTimeout);
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
        auto soap = std::make_shared<soap::Client>(*m_io);
        soap->renewSubscription(publisherUrl, subscriptionId, timeout, [this, soap, cb] (const std::error_code& error, http::StatusCode status, std::string subId, std::chrono::seconds subTimeout) {
            if (cb)
            {
                cb(httpStatusToStatus(error, status), subId, subTimeout);
            }
        });
    });
}

void Client::unsubscribeFromService(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(Status status)> cb)
{
    m_io->post([=] () {
        auto soap = std::make_shared<soap::Client>(*m_io);
        soap->unsubscribe(publisherUrl, subscriptionId, [this, soap, cb, subscriptionId] (const std::error_code& error, http::StatusCode status) {
            if (cb)
            {
                cb(httpStatusToStatus(error, status));
            }

            m_eventCallbacks.erase(subscriptionId);
        });
    });
}

void Client::sendAction(const Action& action, std::function<void(Status, soap::ActionResult)> cb)
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    auto env = std::make_shared<std::string>(action.toString());
    m_io->post([this, url = action.getUrl(), name = action.getName(), urn = action.getServiceTypeUrn(), env, cb = std::move(cb)] () {
        auto soap = std::make_shared<soap::Client>(*m_io);
        soap->action(url, name, urn, *env, [cb, env, soap] (const std::error_code& error, soap::ActionResult result) {
            cb(httpStatusToStatus(error, result.response.status), std::move(result));
        });
    });

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
}

void Client::getFile(const std::string& url, std::function<void(Status, std::string contents)> cb)
{
    http::get(*m_io, url, [cb] (const std::error_code& error, http::Response response) {
        cb(httpStatusToStatus(error, response.status), std::move(response.body));
    });
}

void Client::dispatch(std::function<void()> func)
{
    if (!m_io)
    {
        return;
    }

    m_io->post(func);
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
