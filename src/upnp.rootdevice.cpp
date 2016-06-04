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
#include "upnp/upnp.http.server.h"
#include "upnp/upnp.http.client.h"

#include "upnp.soap.parseutils.h"
#include "guid.h"

#include <regex>

using namespace utils;

namespace upnp
{

static const std::string s_response =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.1\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
    "\r\n"
    "{}";

static const std::string s_errorResponse =
    "HTTP/1.1 500 Internal Server Error\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.1\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
    "\r\n"
    "{}";

static const std::string s_subscriptionResponse =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.1\r\n"
    "SID: {}\r\n"
    "CONTENT-LENGTH: 0\r\n"
    "TIMEOUT: Second-{}\r\n"
    "\r\n";

static const std::string s_notification =
    "NOTIFY {} HTTP/1.1\r\n"
    "HOST: {}\r\n"
    "CONTENT-TYPE: text/xml; charset=\"utf-8\"r\n"
    "NT: upnp:event\r\n"
    "NTS: upnp:propchange\r\n"
    "SID: {}\r\n"
    "SEQ: {}\r\n"
    "CONTENT-LENGTH: bytes in body\r\n"
    "\r\n";

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

    m_httpClient = std::make_unique<http::Client>(m_loop);

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

    m_httpClient.reset();
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

void RootDevice2::addFileToHttpServer(const std::string& path, const std::string& contentType, const std::string& data)
{
    m_httpServer->addFile(path, contentType, data);
}

void RootDevice2::addFileToHttpServer(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data)
{
    m_httpServer->addFile(path, contentType, data);
}

void RootDevice2::removeFileFromHttpServer(const std::string& path)
{
    m_httpServer->removeFile(path);
}

void RootDevice2::notifyEvent(const std::string& serviceId, std::string eventData)
{
    auto iter = m_subscriptions.find(serviceId);
    if (iter != m_subscriptions.end())
    {
        if (iter->second.sequence == std::numeric_limits<uint32_t>::max())
        {
            iter->second.sequence = 1;
        }
        else
        {
            ++(iter->second.sequence);
        }

        auto data = std::make_shared<std::string>(std::move(eventData));
        m_httpClient->notify(iter->second.deliveryUrl, iter->first, iter->second.sequence, *data, [this, data] (uint32_t status, std::string) {
            if (status != 200)
            {
                log::warn("Failed to send notification: HTTP {}", status);
            }
        });
    }
}

std::string RootDevice2::onSubscriptionRequest(http::Parser& parser) noexcept
{
    try
    {
        auto& sid = parser.headerValue("SID");
        if (sid.empty())
        {
            // New subscription request
            GuidGenerator generator;
            SubscriptionRequest request;
            request.sid = fmt::format("uuid:{}", generator.newGuid());
            request.timeout = soap::parseTimeout(parser.headerValue("TIMEOUT"));

            log::info("Subscription request: timeout {}s", request.timeout.count());

            auto response = EventSubscriptionRequested(request);

            SubscriptionData data;
            data.deliveryUrl = parser.headerValue("CALLBACK");
            data.expirationTime = std::chrono::steady_clock::now() + response.timeout;

            m_subscriptions.emplace(request.sid, std::move(data));
            return fmt::format(s_subscriptionResponse, request.sid, response.timeout.count());
        }
        else
        {
            // Subscription renewal
            auto timeout = soap::parseTimeout(parser.headerValue("TIMEOUT"));

            log::info("Subscription renewal");
            auto iter = m_subscriptions.find(sid);
            if (iter != m_subscriptions.end())
            {
                iter->second.expirationTime = std::chrono::steady_clock::now() + timeout;
                return fmt::format(s_subscriptionResponse, sid, timeout.count());
            }

            throw std::runtime_error("");
        }
    }
    catch (std::exception&)
    {
        // TODO: error response
        return "";
    }
}

std::string RootDevice2::onUnsubscriptionRequest(http::Parser& parser) noexcept
{
    log::info("Unsubscription request");
    if (m_subscriptions.erase(parser.headerValue("SID")) > 0)
    {
        return "HTTP/1.1 200 OK";
    }
    else
    {
        static const std::string errorBody =
            "<?xml version=\"1.0\"?>"
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
            "    <s:Body>"
            "        <s:Fault>"
            "            <faultcode>s:Client</faultcode>"
            "            <faultstring>UPnPError</faultstring>"
            "            <detail>"
            "                <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
            "                    <errorCode>412</errorCode>"
            "                    <errorDescription>Precondition Failed</errorDescription>"
            "                </UPnPError>"
            "            </detail>"
            "        </s:Fault>"
            "    </s:Body>"
            "</s:Envelope>";

        return fmt::format(s_errorResponse, errorBody.size(), errorBody);
    }
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

    try
    {
        auto responseBody = ControlActionRequested(request);
        return fmt::format(s_response, responseBody.size(), responseBody);
    }
    catch (std::exception&)
    {
        // Use exeption information to fill in error code and string

        static const std::string errorBody =
            "<?xml version=\"1.0\"?>"
            "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
            "    <s:Body>"
            "        <s:Fault>"
            "            <faultcode>s:Client</faultcode>"
            "            <faultstring>UPnPError</faultstring>"
            "            <detail>"
            "                <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
            "                    <errorCode>403</errorCode>"
            "                    <errorDescription>Invalid request</errorDescription>"
            "                </UPnPError>"
            "            </detail>"
            "        </s:Fault>"
            "    </s:Body>"
            "</s:Envelope>";

        return fmt::format(s_errorResponse, errorBody.size(), errorBody);
    }
}

}
