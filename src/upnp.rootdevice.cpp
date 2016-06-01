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


#include "upnp/upnp.rootdevice.h"

#include "utils/log.h"

#include "upnp/upnp.http.parser.h"
#include "upnp.soap.parseutils.h"

#include <regex>

using namespace utils;

namespace upnp
{

RootDevice2::RootDevice2(std::chrono::seconds advertiseInterval)
: m_advertiseInterval(advertiseInterval)
{
}

RootDevice2::~RootDevice2() noexcept
{
    // try
    // {
    //     uninitialize();
    // }
    // catch (std::exception&) {}
}

void RootDevice2::initialize(const std::string& interfaceName)
{
    auto addr = uv::Address::createIp4(interfaceName);
    addr.setPort(0);

    m_httpServer = std::make_unique<http::Server>(m_loop, addr);
    m_httpServer->setRequestHandler(http::Method::Subscribe, [this] (http::Parser& parser) { return onSubscriptionRequest(parser); });
    m_httpServer->setRequestHandler(http::Method::Unsubscribe, [this] (http::Parser& parser) { return onUnsubscriptionRequest(parser); });
    m_httpServer->setRequestHandler(http::Method::Post, [this] (http::Parser& parser) { return onActionRequest(parser); });

    m_ssdpServer = std::make_unique<ssdp::Server>(m_loop);

    m_thread = std::make_unique<std::thread>([this] () {
        m_loop.run(upnp::uv::RunMode::Default);
    });
}

void RootDevice2::uninitialize()
{
    log::debug("Uninitializing UPnP Root device: {}", m_device.friendlyName);
    uv::asyncSend(m_loop, [this] () {
        m_httpServer->stop([this] () {
            m_ssdpServer->stop([this] () {
                stopLoopAndCloseRequests(m_loop);
            });
        });
    });

    m_thread->join();

    m_httpServer.reset();
    m_ssdpServer.reset();
    m_thread.reset();
}

std::string RootDevice2::getWebrootUrl()
{
    return m_httpServer->getWebRootUrl();
}

void RootDevice2::registerDevice(const std::string& deviceDescriptionXml, const Device& dev)
{
    uv::asyncSend(m_loop, [=] () {
        m_httpServer->addFile(dev.location, "text/xml", deviceDescriptionXml);

        m_device = dev;
        m_device.location = getWebrootUrl() + dev.location;
        m_ssdpServer->run(m_device, m_advertiseInterval);
    });
}

std::string RootDevice2::getUniqueDeviceName()
{
    return m_device.udn;
}

void RootDevice2::acceptSubscription(const std::string& /*serviceId*/, const std::string& /*subscriptionId*/, const std::string& /*response*/)
{
}

void RootDevice2::notifyEvent(const std::string& /*serviceId*/, const std::string& /*response*/)
{
}

std::string RootDevice2::onSubscriptionRequest(http::Parser& parser)
{
    SubscriptionRequest request;
    request.timeout = soap::parseTimeout(parser.headerValue("TIMEOUT"));

    log::info("Subscription request: timeout {}s", request.timeout.count());
    return EventSubscriptionRequested2(request);
}

std::string RootDevice2::onUnsubscriptionRequest(http::Parser& /*parser*/)
{
    log::info("Unsubscription request");
    return "";
}

std::string RootDevice2::onActionRequest(http::Parser& parser)
{
    ActionRequest request;

    auto& action = parser.headerValue("SOAPACTION");
    log::info("Action request: {}", parser.stealBody());

    try
    {
        std::regex re(R"(\"(.*)#(.*)\")");
        std::smatch match;
        if (std::regex_match(action, match, re))
        {
            request.serviceType = match.str(1);
            request.actionName = match.str(2);
        }
        else
        {
            throw std::runtime_error("Failed to parse Soap action: " + action);
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse Soap action: {}", e.what()));
    }

    request.action = parser.stealBody();
    return ControlActionRequested2(request);
}

}
