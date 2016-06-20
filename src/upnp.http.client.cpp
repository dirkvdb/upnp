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

#include "upnp/upnp.http.client.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include "utils/format.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.soap.parseutils.h"

#include "URI.h"
#include "stringview.h"


namespace std
{
 
error_code make_error_code(upnp::http::errc::HttpError e) noexcept
{
    return error_code(static_cast<int>(e), upnp::http::errc::HttpErrorCategory::get());
}
 
template<>
struct is_error_code_enum<upnp::http::errc::HttpError> : std::true_type
{
};
 
}


namespace upnp
{
namespace http
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
: m_timer(io)
, m_socket(io)
, m_timeout(5000ms)
, m_parser(Type::Response)
{
}

Client::~Client() noexcept
{
}

void Client::setTimeout(std::chrono::milliseconds timeout) noexcept
{
    m_timeout = timeout;
}

void Client::performRequest(const ip::tcp::endpoint& addr, std::function<void(const std::error_code&)> cb)
{
    performRequest(addr, std::vector<const_buffer>{ buffer(m_headers) }, cb);
}

void Client::performRequest(const ip::tcp::endpoint& addr, const std::string& body, std::function<void(const std::error_code&)> cb)
{
    performRequest(addr, std::vector<const_buffer>{ buffer(m_headers), buffer(body) }, cb);
}

void Client::performRequest(const ip::tcp::endpoint& addr, const std::vector<const_buffer>& buffers, std::function<void(const std::error_code&)> cb)
{
    m_socket.close();
    m_socket.open(addr.protocol());
    m_socket.async_connect(addr, [this, cb, buffers] (const asio::error_code& error) {
        if (error)
        {
            cb(error);
            return;
        }
        
        m_socket.async_send(buffers, [this, cb] (const std::error_code& error, size_t) {
            if (error)
            {
                cb(error);
                return;
            }
            
            m_socket.async_receive(buffer(m_buffer), [this, cb] (const std::error_code& error, size_t bytesReceived) {
                if (error)
                {
                    cb(error);
                    return;
                }
                
                try
                {
                    auto processed = m_parser.parse(m_buffer.data(), bytesReceived);
                    if (processed != bytesReceived)
                    {
                        log::warn("Failed to parser received http data");
                    }
                }
                catch (const std::exception& e)
                {
                    cb(std::make_error_code(errc::InvalidResponse));
                }
            });
        });
    });
}

void Client::getContentLength(const std::string& url, std::function<void(const std::error_code&, size_t)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("HEAD {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setHeadersCompletedCallback([this, cb] () {
        try
        {
            size_t contentLength = 0;
            if (m_parser.getStatus() == 200)
            {
                contentLength = std::stoul(m_parser.headerValue("Content-length"));
            }
            
            cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), contentLength);
        }
        catch (const std::exception& e)
        {
            cb(std::make_error_code(errc::InvalidResponse), 0);
        }
    });
    
    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, 0);
        }
    });
}

void Client::get(const std::string& url, std::function<void(const std::error_code&, std::string)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), m_parser.stealBody());
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "");
        }
    });
}

void Client::get(const std::string& url, std::function<void(const std::error_code&, std::vector<uint8_t>)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setCompletedCallback([this, cb] () {
        auto body = m_parser.stealBody();
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), std::vector<uint8_t>(body.begin(), body.end()));
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, {});
        }
    });
}

void Client::getRange(const std::string& url, uint64_t offset, uint64_t size, std::function<void(const std::error_code&, std::vector<uint8_t>)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("Range:{}-{}\r\n", offset, offset + size));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        auto body = m_parser.stealBody();
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), std::vector<uint8_t>(body.begin(), body.end()));
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, {});
        }
    });
}

void Client::get(const std::string& url, uint8_t* data, std::function<void(const std::error_code&, uint8_t*)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setCompletedCallback([this, cb, data] () {
        auto body = m_parser.stealBody();
        memcpy(data, body.data(), body.size());
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), data);
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, {});
        }
    });
}

void Client::getRange(const std::string& url, uint64_t offset, uint64_t size, uint8_t* data, std::function<void(const std::error_code&, uint8_t*)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("Range:{}-{}\r\n", offset, offset + size));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb, data] () {
        auto body = m_parser.stealBody();
        memcpy(data, body.data(), body.size());
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), data);
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, {});
        }
    });
}

void Client::subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                       std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("SUBSCRIBE {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("CALLBACK:{}\r\n", callbackUrl));
    m_headers.emplace_back(fmt::format("NT:upnp:event\r\n"));
    m_headers.emplace_back(createTimeoutHeader(timeout));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        try
        {
            cb(std::make_error_code(errc::HttpError(m_parser.getStatus())),
               m_parser.headerValue("sid"),
               soap::parseTimeout(m_parser.headerValue("timeout")),
               m_parser.stealBody());
        }
        catch (const std::exception&)
        {
            cb(std::make_error_code(errc::InvalidResponse), "", 0s, "");
        }
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "", 0s, "");
        }
    });
}

void Client::renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                               std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("SUBSCRIBE {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("SID:{}\r\n", sid));
    m_headers.emplace_back(createTimeoutHeader(timeout));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        try
        {
            cb(std::make_error_code(errc::HttpError(m_parser.getStatus())),
               m_parser.headerValue("sid"),
               soap::parseTimeout(m_parser.headerValue("timeout")),
               m_parser.stealBody());
        }
        catch (const std::exception&)
        {
            cb(std::make_error_code(errc::InvalidResponse), "", 0s, "");
        }
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "", 0s, "");
        }
    });
}

void Client::unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, std::string response)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("UNSUBSCRIBE {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("SID:{}\r\n", sid));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), m_parser.stealBody());
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "");
        }
    });
}

void Client::notify(const std::string& url,
                    const std::string& sid,
                    uint32_t seq,
                    const std::string& body,
                    std::function<void(const std::error_code&, std::string response)> cb)
{
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("NOTIFY {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back("NT:upnp:event\r\n");
    m_headers.emplace_back("NTS:upnp:propchange\r\n");
    m_headers.emplace_back(fmt::format("SID:{}\r\n", sid));
    m_headers.emplace_back(fmt::format("SEQ:{}\r\n", seq));
    m_headers.emplace_back("Content-Type:text/xml\r\n");
    m_headers.emplace_back(fmt::format("Content-Length:{}\r\n", body.size()));
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), m_parser.stealBody());
    });

    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), body, [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "");
        }
    });
}

void Client::soapAction(const std::string& url,
                        const std::string& actionName,
                        const std::string& serviceName,
                        const std::string& envelope,
                        std::function<void(const std::error_code&, std::string)> cb)
{
    //TODO: if the return code is "405 Method Not Allowed" retry but with M-POST as request and additional MAN header
    
    URI uri(url);
    m_headers.clear();
    m_headers.emplace_back(fmt::format("GET {} HTTP/1.1\r\n", uri.getPath()));
    m_headers.emplace_back(fmt::format("Host:{}\r\n", uri.getAuthority()));
    m_headers.emplace_back(fmt::format("SOAPACTION:\"{}#{}\"\r\n", serviceName, actionName));
    m_headers.emplace_back("CONTENT-TYPE: text/xml; charset=\"utf-8\"");
    //m_headers.emplace_back("Connection:Keep-alive\r\n");
    m_headers.emplace_back("\r\n");
    
    m_parser.setBodyDataCallback(nullptr);
    m_parser.setCompletedCallback([this, cb] () {
        cb(std::make_error_code(errc::HttpError(m_parser.getStatus())), m_parser.stealBody());
    });
    
    performRequest(ip::tcp::endpoint(ip::address::from_string(uri.getHost()), uri.getPort()), envelope, [cb] (const std::error_code& error) {
        if (error)
        {
            cb(error, "");
        }
    });
}

}
}
