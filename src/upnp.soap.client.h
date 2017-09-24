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

#include <chrono>
#include <string>
#include <memory>
#include <functional>
#include <system_error>

#include "upnp/asio.h"
#include "upnp/upnp.coroutine.h"
#include "upnp/upnp.soap.types.h"

namespace upnp
{

namespace http
{
    class Client;
    struct Response;
}

namespace soap
{

struct SubscriptionResponse
{
    http::StatusCode statusCode;
    std::string subId;
    std::chrono::seconds timeout;
};

class Client
{
public:
    Client(asio::io_service& io);
    ~Client();

    void setTimeout(std::chrono::milliseconds timeout) noexcept;

    void subscribe(const std::string& url,
                   const std::string& callbackUrl,
                   std::chrono::seconds timeout,
                   std::function<void(const std::error_code&, http::StatusCode, std::string subId, std::chrono::seconds timeout)> cb);

    void renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                           std::function<void(const std::error_code&, http::StatusCode, std::string subId, std::chrono::seconds timeout)> cb);
    void unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, http::StatusCode)> cb);

    void notify(const std::string& url, const std::string& sid, uint32_t seq, const std::string& body, std::function<void(const std::error_code&, http::StatusCode)> cb);

    void action(const std::string& url,
                const std::string& actionName,
                const std::string& serviceName,
                const std::string& envelope,
                std::function<void(const std::error_code&, ActionResult response)> cb);

    Future<SubscriptionResponse> subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout);
    Future<SubscriptionResponse> renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout);
    Future<http::StatusCode> unsubscribe(const std::string& url, const std::string& sid);
    Future<http::StatusCode> notify(const std::string& url,
                                    const std::string& sid,
                                    uint32_t seq,
                                    const std::string& body);

    Future<ActionResult> action(const std::string& url,
                                const std::string& actionName,
                                const std::string& serviceName,
                                const std::string& envelope);

private:
    std::unique_ptr<http::Client> m_httpClient;
};

}
}
