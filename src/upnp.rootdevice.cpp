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
#include "upnp/upnp.http.types.h"

#include "utils/log.h"

#include "upnp.http.client.h"
#include "upnp.http.utils.h"
#include "upnp.soap.client.h"
#include "upnp/upnp.asio.h"
#include "upnp/upnp.http.server.h"
#include "upnp/upnp.servicefaults.h"

#include "guid.h"
#include "upnp.soap.parseutils.h"

#include <regex>

namespace upnp
{

using namespace asio;
using namespace utils;
using namespace std::chrono_literals;

static const std::string s_htmlErrorResponse =
    "HTTP/1.1 {} {}\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.1\r\n"
    "CONTENT-LENGTH: 0\r\n"
    "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
    "\r\n";

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

static const std::string s_errorBody =
    "<?xml version=\"1.0\"?>"
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body>"
    "<s:Fault>"
    "<faultcode>s:Client</faultcode>"
    "<faultstring>UPnPError</faultstring>"
    "<detail>"
    "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
    "<errorCode>{}</errorCode>"
    "<errorDescription>{}</errorDescription>"
    "</UPnPError>"
    "</detail>"
    "</s:Fault>"
    "</s:Body>"
    "</s:Envelope>";

static const std::string s_errorBodyNoDescription =
    "<?xml version=\"1.0\"?>"
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body>"
    "<s:Fault>"
    "<faultcode>s:Client</faultcode>"
    "<faultstring>UPnPError</faultstring>"
    "<detail>"
    "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
    "<errorCode>{}</errorCode>"
    "</UPnPError>"
    "</detail>"
    "</s:Fault>"
    "</s:Body>"
    "</s:Envelope>";

namespace
{

std::vector<std::string> parseCallback(const std::string_view& cb)
{
    if (cb.empty() || cb.size() < 2 || cb[0] != '<' || cb[cb.size() - 1] != '>')
    {
        log::warn("Invalid CALLBACK value: {}", cb);
        return {};
    }

    return str::split(cb.substr(1, cb.size() - 2), ',');
}

std::string faultToString(const soap::Fault& fault)
{
    auto& errorMsg = fault.errorDescription();
    if (errorMsg.empty())
    {
        return fmt::format(s_errorBodyNoDescription, fault.errorCode());
    }
    else
    {
        return fmt::format(s_errorBody, fault.errorCode(), fault.errorDescription());
    }
}

std::string createSoapErrorResponse(const soap::Fault& fault)
{
    auto faultString = faultToString(fault);
    return fmt::format(s_errorResponse, faultString.size(), faultString);
}

std::string createErrorResponse(const soap::Fault& fault)
{
    return fmt::format(s_htmlErrorResponse, fault.errorCode(), fault.errorDescription());
}
}

RootDevice::RootDevice(std::chrono::seconds advertiseInterval)
: m_owningIo(std::make_unique<asio::io_service>())
, m_io(*m_owningIo)
, m_advertiseInterval(advertiseInterval)
{
}

RootDevice::RootDevice(std::chrono::seconds advertiseInterval, asio::io_service& io)
: m_io(io)
, m_advertiseInterval(advertiseInterval)
{
}

RootDevice::~RootDevice() noexcept = default;

void RootDevice::initialize()
{
    initialize(ip::tcp::endpoint(ip::address_v4::any(), 0));
}

void RootDevice::initialize(const std::string& interfaceName)
{
    auto addr = getNetworkInterfaceV4(interfaceName).address;
    initialize(ip::tcp::endpoint(addr, 0));
}

void RootDevice::initialize(const ip::tcp::endpoint& endPoint)
{
    m_httpServer = std::make_unique<http::Server>(m_io);

    m_httpServer->start(endPoint);
    m_httpServer->setRequestHandler(http::Method::Subscribe, [this](const http::Request& request) { return onSubscriptionRequest(request); });
    m_httpServer->setRequestHandler(http::Method::Unsubscribe, [this](const http::Request& request) { return onUnsubscriptionRequest(request); });
    m_httpServer->setRequestHandler(http::Method::Post, [this](const http::Request& request) { return onActionRequest(request); });

    m_ssdpServer = std::make_unique<ssdp::Server>(m_io);

    if (m_owningIo)
    {
        // this rootdevice is the owner of the io service, so run it in a thread
        m_asioThread = std::make_unique<std::thread>([this]() {
            asio::io_service::work work(*m_owningIo);
            m_owningIo->run();
        });
    }
}

void RootDevice::uninitialize()
{
    log::debug("Uninitializing UPnP Root device: {}", m_device.friendlyName);

    m_io.post([this]() {
        m_httpServer->stop();
        m_ssdpServer->stop([this]() {
            m_io.stop();
        });
    });

    if (m_asioThread)
    {
        m_asioThread->join();
        m_asioThread.reset();

        m_httpServer.reset();
        m_ssdpServer.reset();
    }
}

std::string RootDevice::getWebrootUrl()
{
    return m_httpServer->getWebRootUrl();
}

void RootDevice::registerDevice(const std::string& deviceDescriptionXml, const Device& dev)
{
    m_io.post([=]() {
        m_httpServer->addFile(dev.location, "text/xml", deviceDescriptionXml);

        m_device          = dev;
        m_device.location = getWebrootUrl() + dev.location;
        m_ssdpServer->run(m_device, m_advertiseInterval);
    });
}

std::string RootDevice::getUniqueDeviceName()
{
    return m_device.udn;
}

void RootDevice::addFileToHttpServer(const std::string& path, const std::string& contentType, const std::string& data)
{
    m_httpServer->addFile(path, contentType, data);
}

void RootDevice::addFileToHttpServer(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data)
{
    m_httpServer->addFile(path, contentType, data);
}

void RootDevice::removeFileFromHttpServer(const std::string& path)
{
    m_httpServer->removeFile(path);
}

void RootDevice::notifyEvent(const std::string& serviceId, std::string eventData)
{
    auto iter = m_subscriptions.find(serviceId);
    if (iter != m_subscriptions.end())
    {
        auto seq = iter->second.sequence;

        if (iter->second.sequence == std::numeric_limits<uint32_t>::max())
        {
            iter->second.sequence = 1;
        }
        else
        {
            ++(iter->second.sequence);
        }

        auto soapClient = std::make_shared<soap::Client>(m_io);
        auto data       = std::make_shared<std::string>(std::move(eventData));
        log::debug("Send notification: {} {}", iter->second.deliveryUrls.front(), *data);
        soapClient->notify(iter->second.deliveryUrls.front(), iter->first, seq, *data, [data, soapClient](const std::error_code& error, http::StatusCode status) {
            if (error)
            {
                log::warn("Failed to send notification: HTTP {}", error.message());
            }
            else
            {
                log::debug("Notification sent: HTTP status {}", status);
            }
        });
    }
}

std::string RootDevice::onSubscriptionRequest(const http::Request& httpReq) noexcept
{
    try
    {
        auto sid = httpReq.field("SID");
        if (sid.empty())
        {
            // New subscription request
            GuidGenerator       generator;
            SubscriptionRequest request;
            request.url     = httpReq.url();
            request.sid     = fmt::format("uuid:{}", generator.newGuid());
            request.timeout = soap::parseTimeout(httpReq.field("TIMEOUT"));

            log::info("Subscription request: timeout {}s CB: {}", request.timeout.count(), httpReq.field("CALLBACK"));

            SubscriptionData data;
            data.deliveryUrls = parseCallback(httpReq.field("CALLBACK"));

            if (data.deliveryUrls.empty())
            {
                throw PreconditionFailed();
            }

            auto response       = EventSubscriptionRequested(request);
            data.expirationTime = std::chrono::steady_clock::now() + response.timeout;
            m_subscriptions.emplace(request.sid, std::move(data));

            if (!response.initialEvent.empty())
            {
                // Send the initial event
                m_io.post([=]() {
                    notifyEvent(request.sid, response.initialEvent);
                });
            }

            return fmt::format(s_subscriptionResponse, request.sid, response.timeout.count());
        }
        else
        {
            // Subscription renewal
            auto timeout = soap::parseTimeout(httpReq.field("TIMEOUT"));

            log::info("Subscription renewal");
            auto iter = m_subscriptions.find(std::string(sid));
            if (iter != m_subscriptions.end())
            {
                iter->second.expirationTime = std::chrono::steady_clock::now() + timeout;
                return fmt::format(s_subscriptionResponse, sid, timeout.count());
            }

            throw PreconditionFailed();
        }
    }
    catch (soap::Fault& fault)
    {
        return createErrorResponse(fault);
    }
    catch (std::exception& e)
    {
        return createErrorResponse(PreconditionFailed());
    }
}

std::string RootDevice::onUnsubscriptionRequest(const http::Request& request) noexcept
{
    log::info("Unsubscription request");
    if (m_subscriptions.erase(std::string(request.field("SID"))) > 0)
    {
        return http::okResponse();
    }
    else
    {
        return createErrorResponse(PreconditionFailed());
    }
}

std::string RootDevice::onActionRequest(const http::Request& httpReq)
{
    try
    {
        ActionRequest request;
        request.action = httpReq.body();
        log::debug("Action request: {}", request.action);

        try
        {
            std::tie(request.serviceType, request.actionName) = soap::parseAction(httpReq.field("SOAPACTION"));
        }
        catch (const std::runtime_error& e)
        {
            log::error(e.what());
            throw InvalidAction();
        }

        auto responseBody = ControlActionRequested(request);
        return http::createResponse(http::StatusCode::Ok, responseBody);
    }
    catch (soap::Fault& fault)
    {
        return createSoapErrorResponse(fault);
    }
    catch (std::exception& e)
    {
        return createSoapErrorResponse(ActionFailed());
    }
}

asio::io_service& RootDevice::ioService() noexcept
{
    return m_io;
}
}
