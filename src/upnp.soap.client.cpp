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

#include "upnp.soap.client.h"

#include "utils/format.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.soap.parseutils.h"
#include "upnp.http.client.h"

#include "URI.h"
#include "stringview.h"

#include <cassert>
#include <stdexcept>
#include <sstream>


namespace upnp
{
namespace soap
{

using namespace asio;
using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace
{

std::string createTimeoutHeader(std::chrono::seconds timeout)
{
    if (timeout.count() == 0)
    {
        return "TIMEOUT: Second-infinite\r\n";
    }
    else
    {
        return fmt::format("TIMEOUT: Second-{}\r\n", timeout.count());
    }
}

}

Client::Client(asio::io_service& io)
: m_httpClient(std::make_unique<http::Client>(io))
{
}

Client::~Client() = default;

void Client::setTimeout(std::chrono::milliseconds timeout) noexcept
{
    m_httpClient->setTimeout(timeout);
}

void Client::subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                       std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader(fmt::format("CALLBACK:<{}>\r\n", callbackUrl));
    m_httpClient->addHeader(fmt::format("NT:upnp:event\r\n"));
    m_httpClient->addHeader(createTimeoutHeader(timeout));

    m_httpClient->perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, std::string response) {
        try
        {
            if (ec.value() != http::error::Ok)
            {
                cb(ec, "", 0s, "");
                return;
            }

            cb(ec,
               m_httpClient->getResponseHeaderValue("sid"),
               soap::parseTimeout(m_httpClient->getResponseHeaderValue("timeout")),
               std::move(response));
        }
        catch (const std::exception& e)
        {
            log::error("Subscribe error: {}", e.what());
            cb(std::make_error_code(http::error::InvalidResponse), "", 0s, "");
        }
    });
}

void Client::renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                               std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader(fmt::format("SID:{}\r\n", sid));
    m_httpClient->addHeader(createTimeoutHeader(timeout));

    m_httpClient->perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, std::string response) {
        try
        {
            if (ec.value() != http::error::Ok)
            {
                cb(ec, "", 0s, "");
                return;
            }

            cb(ec,
               m_httpClient->getResponseHeaderValue("sid"),
               soap::parseTimeout(m_httpClient->getResponseHeaderValue("timeout")),
               std::move(response));
        }
        catch (const std::exception& e)
        {
            log::error("Renew Subscription error: {}", e.what());
            cb(std::make_error_code(http::error::InvalidResponse), "", 0s, "");
        }
    });
}

void Client::unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, std::string response)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader(fmt::format("SID:{}\r\n", sid));

    m_httpClient->perform(http::Method::Unsubscribe, [this, cb] (const std::error_code& ec, std::string response) {
        cb(ec, std::move(response));
    });
}

void Client::notify(const std::string& url,
                    const std::string& sid,
                    uint32_t seq,
                    const std::string& body,
                    std::function<void(const std::error_code&, std::string response)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("NT:upnp:event\r\n");
    m_httpClient->addHeader("NTS:upnp:propchange\r\n");
    m_httpClient->addHeader(fmt::format("SID:{}\r\n", sid));
    m_httpClient->addHeader(fmt::format("SEQ:{}\r\n", seq));
    m_httpClient->addHeader("Content-Type:text/xml\r\n");
    m_httpClient->addHeader(fmt::format("Content-Length:{}\r\n", body.size()));

    m_httpClient->perform(http::Method::Notify, body, [this, cb] (const std::error_code& ec, std::string response) {
        cb(ec, std::move(response));
    });
}

void Client::action(const std::string& url,
                    const std::string& actionName,
                    const std::string& serviceName,
                    const std::string& envelope,
                    std::function<void(const std::error_code&, ActionResult)> cb)
{
    //TODO: if the return code is "405 Method Not Allowed" retry but with M-POST as request and additional MAN header

    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("NT:upnp:event\r\n");
    m_httpClient->addHeader(fmt::format("SOAPACTION:\"{}#{}\"\r\n", serviceName, actionName));
    m_httpClient->addHeader("Content-Type: text/xml; charset=\"utf-8\"\r\n");
    m_httpClient->addHeader(fmt::format("Content-Length:{}\r\n", envelope.size()));

    m_httpClient->perform(http::Method::Post, envelope, [this, cb] (const std::error_code& ec, std::string response) {
        if (ec.value() != http::error::InternalServerError)
        {
            cb(ec, ActionResult(std::move(response)));
        }
        else
        {
            try
            {
                auto fault = soap::parseFault(response);
                cb(ec, ActionResult(std::move(response), std::move(fault)));
            }
            catch (std::exception& e)
            {
                log::warn("Failed to parse soap fault message: {}", e.what());
                cb(ec, ActionResult(std::move(response)));
            }
        }
    });
}

}
}
