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

#include "URI.h"

#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string_view>


namespace upnp::soap
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
: _httpClient(io)
{
}

void Client::setTimeout(std::chrono::milliseconds timeout) noexcept
{
    _httpClient.setTimeout(timeout);
}

void Client::subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                       std::function<void(const std::error_code&, http::StatusCode, std::string subId, std::chrono::seconds timeout)> cb)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("CALLBACK", fmt::format("<{}>", callbackUrl));
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    _httpClient.perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, const http::Response& response) {
        try
        {
            std::string_view subId;
            std::chrono::seconds timeout;
            if (response.status == http::StatusCode::Ok)
            {
                subId = _httpClient.getResponseHeaderValue("sid");
                timeout = soap::parseTimeout(_httpClient.getResponseHeaderValue("timeout"));
            }

            cb(ec, response.status, std::string(subId), timeout);
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
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("SID", sid);
    _httpClient.addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    _httpClient.perform(http::Method::Subscribe, [this, cb] (const std::error_code& ec, const http::Response& response) {
        try
        {
            std::string_view subId;
            std::chrono::seconds timeout;
            if (response.status == http::StatusCode::Ok)
            {
                subId = _httpClient.getResponseHeaderValue("sid");
                timeout = soap::parseTimeout(_httpClient.getResponseHeaderValue("timeout"));
            }

            cb(ec, response.status, std::string(subId), timeout);
        }
        catch (const std::exception& e)
        {
            log::error("Renew Subscription error: {}", e.what());
            cb(std::make_error_code(http::error::InvalidResponse), response.status, "", 0s);
        }
    });
}

Future<SubscriptionResponse> Client::subscribe(const std::string& url,
                                               const std::string& callbackUrl,
                                               std::chrono::seconds timeout)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("CALLBACK", fmt::format("<{}>", callbackUrl));
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    auto response = co_await _httpClient.perform(http::Method::Subscribe);

    SubscriptionResponse subResponse;
    subResponse.statusCode = response.status;

    if (response.status == http::StatusCode::Ok)
    {
        try
        {
            subResponse.subId = _httpClient.getResponseHeaderValue("sid");
            subResponse.timeout = soap::parseTimeout(_httpClient.getResponseHeaderValue("timeout"));
        }
        catch (const std::exception& e)
        {
            log::error("Subscribe error: {}", e.what());
            throw std::system_error(std::make_error_code(http::error::InvalidResponse));
        }
    }

    co_return subResponse;
}

Future<SubscriptionResponse> Client::renewSubscription(const std::string& url,
                                                       const std::string& sid,
                                                       std::chrono::seconds timeout)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("SID", sid);
    _httpClient.addHeader("TIMEOUT", createTimeoutHeaderValue(timeout));

    auto response = co_await _httpClient.perform(http::Method::Subscribe);

    SubscriptionResponse subResponse;
    subResponse.statusCode = response.status;

    if (response.status == http::StatusCode::Ok)
    {
        try
        {
            subResponse.subId = _httpClient.getResponseHeaderValue("sid");
            subResponse.timeout = soap::parseTimeout(_httpClient.getResponseHeaderValue("timeout"));
        }
        catch (const std::exception& e)
        {
            log::error("Renew Subscription error: {}", e.what());
            throw std::system_error(std::make_error_code(http::error::InvalidResponse));
        }
    }

    co_return subResponse;
}

void Client::unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, http::StatusCode)> cb)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("SID", sid);

    _httpClient.perform(http::Method::Unsubscribe, [cb] (const std::error_code& ec, const http::Response& response) {
        cb(ec, response.status);
    });
}

void Client::notify(const std::string& url,
                    const std::string& sid,
                    uint32_t seq,
                    const std::string& body,
                    std::function<void(const std::error_code&, http::StatusCode)> cb)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("NTS", "upnp:propchange");
    _httpClient.addHeader("SID", sid);
    _httpClient.addHeader("SEQ", std::to_string(seq));
    _httpClient.addHeader("Content-Type", "text/xml");

    _httpClient.perform(http::Method::Notify, body, [cb] (const std::error_code& ec, const http::Response& response) {
        cb(ec, response.status);
    });
}

Future<http::StatusCode> Client::unsubscribe(const std::string& url, const std::string& sid)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("SID", sid);

    auto response = co_await _httpClient.perform(http::Method::Unsubscribe);
    co_return response.status;
}

Future<http::StatusCode> Client::notify(const std::string& url,
                                        const std::string& sid,
                                        uint32_t seq,
                                        const std::string& body)
{
    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("NTS", "upnp:propchange");
    _httpClient.addHeader("SID", sid);
    _httpClient.addHeader("SEQ", std::to_string(seq));
    _httpClient.addHeader("Content-Type", "text/xml");

    auto response = co_await _httpClient.perform(http::Method::Notify, body);
    co_return response.status;
}

void Client::action(const std::string& url,
                    const std::string& actionName,
                    const std::string& serviceName,
                    const std::string& envelope,
                    std::function<void(const std::error_code&, ActionResult)> cb)
{
    //TODO: if the return code is "405 Method Not Allowed" retry but with M-POST as request and additional MAN header

    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("SOAPACTION", fmt::format("\"{}#{}\"", serviceName, actionName));
    _httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");

    _httpClient.perform(http::Method::Post, envelope, [cb] (const std::error_code& ec, http::Response response) {
        cb(ec, ActionResult(response.status, std::move(response.body)));
    });
}

Future<ActionResult> Client::action(const std::string& url,
                                    const std::string& actionName,
                                    const std::string& serviceName,
                                    const std::string& envelope)
{
    //TODO: if the return code is "405 Method Not Allowed" retry but with M-POST as request and additional MAN header

    _httpClient.reset();

    _httpClient.setUrl(url);
    _httpClient.addHeader("NT", "upnp:event");
    _httpClient.addHeader("SOAPACTION", fmt::format("\"{}#{}\"", serviceName, actionName));
    _httpClient.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");

    auto response = co_await _httpClient.perform(http::Method::Post, envelope);
    co_return ActionResult(response.status, std::move(response.body));
}

}
