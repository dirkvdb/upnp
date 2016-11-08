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

std::string createTimeoutHeaderValue(std::chrono::seconds timeout)
{
    if (timeout == 0s)
    {
        return "Second-infinite";
    }
    else
    {
        return fmt::format("Second-{}", timeout.count());
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
                       std::function<void(const std::error_code&, http::StatusCode, std::string subId, std::chrono::seconds timeout)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("CALLBACK", fmt::format("<{}>", callbackUrl));
    m_httpClient->addHeader("NT", "upnp:event");
    m_httpClient->addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    m_httpClient->perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, const http::Response& response) {
        try
        {
            std::string_view subId;
            std::chrono::seconds timeout;
            if (response.status == http::StatusCode::Ok)
            {
                subId = m_httpClient->getResponseHeaderValue("sid");
                timeout = soap::parseTimeout(m_httpClient->getResponseHeaderValue("timeout"));
            }

            cb(ec, response.status, subId.to_string(), timeout);
        }
        catch (const std::exception& e)
        {
            log::error("Subscribe error: {}", e.what());
            cb(std::make_error_code(http::error::InvalidResponse), response.status, "", 0s);
        }
    });
}

void Client::renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                               std::function<void(const std::error_code&, http::StatusCode, std::string subId, std::chrono::seconds timeout)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("SID", sid);
    m_httpClient->addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    m_httpClient->perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, const http::Response& response) {
        try
        {
            std::string_view subId;
            std::chrono::seconds timeout;
            if (response.status == http::StatusCode::Ok)
            {
                subId = m_httpClient->getResponseHeaderValue("sid");
                timeout = soap::parseTimeout(m_httpClient->getResponseHeaderValue("timeout"));
            }

            cb(ec, response.status, subId.to_string(), timeout);
        }
        catch (const std::exception& e)
        {
            log::error("Renew Subscription error: {}", e.what());
            cb(std::make_error_code(http::error::InvalidResponse), response.status, "", 0s);
        }
    });
}

void Client::unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, http::StatusCode)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("SID", sid);

    m_httpClient->perform(http::Method::Unsubscribe, [this, cb] (const std::error_code& ec, const http::Response& response) {
        cb(ec, response.status);
    });
}

void Client::notify(const std::string& url,
                    const std::string& sid,
                    uint32_t seq,
                    const std::string& body,
                    std::function<void(const std::error_code&, http::StatusCode)> cb)
{
    m_httpClient->reset();

    m_httpClient->setUrl(url);
    m_httpClient->addHeader("NT", "upnp:event");
    m_httpClient->addHeader("NTS", "upnp:propchange");
    m_httpClient->addHeader("SID", sid);
    m_httpClient->addHeader("SEQ", std::to_string(seq));
    m_httpClient->addHeader("Content-Type", "text/xml");

    m_httpClient->perform(http::Method::Notify, body, [this, cb] (const std::error_code& ec, const http::Response& response) {
        cb(ec, response.status);
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
    m_httpClient->addHeader("NT", "upnp:event");
    m_httpClient->addHeader("SOAPACTION", fmt::format("\"{}#{}\"", serviceName, actionName));
    m_httpClient->addHeader("Content-Type", "text/xml; charset=\"utf-8\"");

    m_httpClient->perform(http::Method::Post, envelope, [this, cb] (const std::error_code& ec, http::Response response) {
        if (ec || response.status != http::StatusCode::InternalServerError)
        {
            cb(ec, ActionResult(std::move(response)));
        }
        else
        {
            try
            {
                auto fault = soap::parseFault(response.body);
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
