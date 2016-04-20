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

#include "upnp/upnp.client.h"

#include "utils/log.h"
#include "upnp/upnp.uv.h"
#include "upnp/upnp.action.h"
#include "upnp.gena.server.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;
using namespace std::placeholders;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

Client2::Client2()
: m_loop(std::make_unique<uv::Loop>())
, m_httpClient(*m_loop)
{
}

Client2::~Client2() = default;

void Client2::initialize()
{
    log::debug("Initializing UPnP SDK");
    return initialize(uv::Address::createIp4("0.0.0.0", 0));
}

void Client2::initialize(const std::string& interfaceName, uint16_t port)
{
    log::debug("Initializing UPnP SDK");
    for (auto& intf : uv::getNetworkInterfaces())
    {
        if (intf.isIpv4() && intf.name == interfaceName)
        {
            auto addr = uv::Address::createIp4(intf.address.address4);
            addr.setPort(port);
            return initialize(addr);
        }
    }

    throw std::runtime_error("Could not find network interface with name: " + interfaceName);
}

void Client2::initialize(const uv::Address& addr)
{
    m_eventServer = std::make_unique<gena::Server>(*m_loop, addr, [&] (const SubscriptionEvent& ev) {
        auto iter = m_eventCallbacks.find(ev.sid);
        if (iter != m_eventCallbacks.end())
        {
            iter->second(ev);
        }
    });

    runLoop();
}

void Client2::uninitialize()
{
    log::debug("Uninitializing UPnP SDK");
    uv::asyncSend(*m_loop, [&] () {
        m_eventServer->stop([this] () {
            m_eventServer.reset();
        });
    });

    m_thread->join();
    m_thread.reset();
}

std::string Client2::getIpAddress() const
{
    return m_eventServer->getAddress().ip();
}

uint16_t Client2::getPort() const
{
    return m_eventServer->getAddress().port();
}

void Client2::subscribeToService(const std::string& publisherUrl, std::chrono::seconds timeout, std::function<std::function<void(SubscriptionEvent)>(int32_t, std::string, std::chrono::seconds)> cb)
{
    if (!m_eventServer)
    {
        throw std::runtime_error("UPnP library is not properly initialized");
    }

    auto addr = m_eventServer->getAddress();
    auto eventServerUrl = fmt::format("http://{}:{}/", addr.ip(), addr.port());

    uv::asyncSend(*m_loop, [=] () {
#ifdef DEBUG_UPNP_CLIENT
        log::debug("Subscribe to service: {}", publisherUrl);
#endif
        m_httpClient.subscribe(publisherUrl, eventServerUrl, timeout, [this, cb] (int32_t status, std::string subId, std::chrono::seconds subTimeout, std::string response) {
//#ifdef DEBUG_UPNP_CLIENT
            log::debug("Subscribe response: {}", response);
//#endif
            m_eventCallbacks.emplace(subId, cb(status, subId, subTimeout));
        });
    });
}

void Client2::unsubscribeFromService(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(int32_t status)> cb)
{
    uv::asyncSend(*m_loop, [=] () {
        m_httpClient.unsubscribe(publisherUrl, subscriptionId, [=] (int32_t status, std::string response) {
//#ifdef DEBUG_UPNP_CLIENT
            log::debug("Unsubscribe response: {}", response);
//#endif
            cb(status);
        });
    });
}

void Client2::sendAction(const Action2& action, std::function<void(int32_t, std::string)> cb)
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    m_httpClient.soapAction(action.getUrl(), action.getName(), action.getServiceTypeUrn(), action.toString(), std::move(cb));

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
}

void Client2::runLoop()
{
    m_thread = std::make_unique<std::thread>([this] () {
        m_loop->run(upnp::uv::RunMode::Default);
    });
}

uv::Loop& Client2::loop()
{
    return *m_loop;
}

void Client2::handlEvent(const SubscriptionEvent& event)
{
    auto iter = m_eventCallbacks.find(event.sid);
    if (iter != m_eventCallbacks.end())
    {
        iter->second(event);
    }
}

}
