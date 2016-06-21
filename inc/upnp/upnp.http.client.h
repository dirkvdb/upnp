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

#include <string>
#include <vector>
#include <chrono>
#include <cinttypes>
#include <unordered_map>

#include <asio.hpp>
#include <experimental/string_view>

#include "upnp/upnp.http.parser.h"

namespace upnp
{
namespace http
{

namespace errc
{

enum HttpError
{
    InvalidResponse = -1,
    Continue = 100,
    SwitchingProtocols = 101,
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthorativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultipleChoises = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    Reserved = 306,
    TemporaryRedirect = 307,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestURITooLong = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505
};

inline const char* error_message(int ec)
{
    switch(ec)
    {
    case InvalidResponse: return "Invalid response";
    case Continue: return "Continue";
    case SwitchingProtocols: return "Switching Protocols";
    case Ok: return "OK";
    case Created: return "Created";
    case Accepted: return "Accepted";
    case NonAuthorativeInformation: return "Non-Authoritative Information";
    case NoContent: return "No Content";
    case ResetContent: return "Reset Content";
    case PartialContent: return "Partial Content";
    case MultipleChoises: return "Multiple Choices";
    case MovedPermanently: return "Moved Permanently";
    case Found: return "Found";
    case SeeOther: return "See Other";
    case NotModified: return "Not Modified";
    case UseProxy: return "Use Proxy";
    case TemporaryRedirect: return "Temporary Redirect";
    case BadRequest: return "Bad Request";
    case Unauthorized: return "Unauthorized";
    case PaymentRequired: return "Payment Required";
    case Forbidden: return "Forbidden";
    case NotFound: return "Not Found";
    case MethodNotAllowed: return "Method Not Allowed";
    case NotAcceptable: return "Not Acceptable";
    case ProxyAuthenticationRequired: return "Proxy Authentication Required";
    case RequestTimeout: return "Request Timeout";
    case Conflict: return "Conflict";
    case Gone: return "Gone";
    case LengthRequired: return "Length Required";
    case PreconditionFailed: return "Precondition Failed";
    case RequestEntityTooLarge: return "Request Entity Too Large";
    case RequestURITooLong: return "Request-URI Too Long";
    case UnsupportedMediaType: return "Unsupported Media Type";
    case RequestedRangeNotSatisfiable: return "Requested Range Not Satisfiable";
    case ExpectationFailed: return "Expectation Failed";
    case InternalServerError: return "Internal Server Error";
    case NotImplemented: return "Not Implemented";
    case BadGateway: return "Bad Gateway";
    case ServiceUnavailable: return "Service Unavailable";
    case GatewayTimeout: return "Gateway Timeout";
    case HTTPVersionNotSupported: return "HTTP Version Not Supported";

    case Reserved: return "<reserved>";
    default: return "<unknown-status>";
    }
}

class HttpErrorCategory : public std::error_category
{
public:
    HttpErrorCategory() = default;

    std::string message(int c) const override
    {
        return error_message(c);
    }

    const char* name() const noexcept override
    {
        return "Http Error Category";
    }

    static const std::error_category& get()
    {
        const static HttpErrorCategory cat{};
        return cat;
    }
};

}

class Client
{
public:
    Client(asio::io_service& io);
    Client(const Client&) = delete;
    ~Client() noexcept;

    void setTimeout(std::chrono::milliseconds timeout) noexcept;

    void getContentLength(const std::string& url, std::function<void(const std::error_code&, size_t length)> cb);
    void get(const std::string& url, std::function<void(const std::error_code&, std::string data)> cb);
    void get(const std::string& url, std::function<void(const std::error_code&, std::vector<uint8_t> data)> cb);
    void get(const std::string& url, uint8_t* data, std::function<void(const std::error_code&, uint8_t* data)> cb);

    void getRange(const std::string& url, uint64_t offset, uint64_t size, std::function<void(const std::error_code&, std::vector<uint8_t> data)> cb);
    void getRange(const std::string& url, uint64_t offset, uint64_t size, uint8_t* pData, std::function<void(const std::error_code&, uint8_t* data)> cb);

    void subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                   std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb);
    void renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                           std::function<void(const std::error_code&, std::string subId, std::chrono::seconds timeout, std::string response)> cb);
    void unsubscribe(const std::string& url, const std::string& sid, std::function<void(const std::error_code&, std::string response)> cb);

    void notify(const std::string& url, const std::string& sid, uint32_t seq, const std::string& body, std::function<void(const std::error_code&, std::string response)> cb);

    void soapAction(const std::string& url,
                    const std::string& actionName,
                    const std::string& serviceName,
                    const std::string& envelope,
                    std::function<void(const std::error_code&, std::string data)> cb);

private:
    void reset();

    void performRequest(const asio::ip::tcp::endpoint& addr, std::function<void(const std::error_code&)> cb);
    void performRequest(const asio::ip::tcp::endpoint& addr, const std::string& body, std::function<void(const std::error_code&)> cb);
    void performRequest(const asio::ip::tcp::endpoint& addr, const std::vector<asio::const_buffer>& buffers, std::function<void(const std::error_code&)> cb);
    void receiveData(std::function<void(const std::error_code&)> cb);

    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    std::chrono::milliseconds m_timeout;
    std::vector<std::string> m_headers;
    std::array<char, 2048> m_buffer;
    Parser m_parser;
};

}
}

namespace std
{

error_code make_error_code(upnp::http::errc::HttpError e) noexcept;

}

