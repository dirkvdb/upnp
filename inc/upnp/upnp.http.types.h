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

#include "stringview.h"
#include "upnp/upnp.utils.h"

#include <boost/beast/http.hpp>
#include <boost/utility/string_view.hpp>
#include <ostream>
#include <string>
#include <system_error>

namespace upnp
{
namespace http
{

namespace beast = boost::beast;

enum class Method : uint8_t
{
    Notify,
    Search,
    Subscribe,
    Unsubscribe,
    Get,
    Head,
    Post,
    Unknown
};

Method methodFromString(const std::string_view& str);

namespace error
{

enum ErrorCode
{
    InvalidResponse = -1,
    NetworkError = -2,
    Timeout = -3,
    Unexpected = -4
};

inline const char* error_message(ErrorCode error)
{
    switch (error)
    {
    case ErrorCode::InvalidResponse: return "Invalid response";
    case ErrorCode::NetworkError: return "Network error";
    case ErrorCode::Timeout: return "Timeout";
    case ErrorCode::Unexpected: return "Unexpected";
    default: return "<unknown-error>";
    }
}

class HttpErrorCategory : public std::error_category
{
public:
    HttpErrorCategory() = default;

    std::string message(int c) const override
    {
        return error_message(ErrorCode(c));
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

enum class StatusCode : uint32_t
{
    None = 0,
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

inline const char* status_message(StatusCode status)
{
    switch (status)
    {
    case StatusCode::Continue: return "Continue";
    case StatusCode::SwitchingProtocols: return "Switching Protocols";
    case StatusCode::Ok: return "OK";
    case StatusCode::Created: return "Created";
    case StatusCode::Accepted: return "Accepted";
    case StatusCode::NonAuthorativeInformation: return "Non-Authoritative Information";
    case StatusCode::NoContent: return "No Content";
    case StatusCode::ResetContent: return "Reset Content";
    case StatusCode::PartialContent: return "Partial Content";
    case StatusCode::MultipleChoises: return "Multiple Choices";
    case StatusCode::MovedPermanently: return "Moved Permanently";
    case StatusCode::Found: return "Found";
    case StatusCode::SeeOther: return "See Other";
    case StatusCode::NotModified: return "Not Modified";
    case StatusCode::UseProxy: return "Use Proxy";
    case StatusCode::TemporaryRedirect: return "Temporary Redirect";
    case StatusCode::BadRequest: return "Bad Request";
    case StatusCode::Unauthorized: return "Unauthorized";
    case StatusCode::PaymentRequired: return "Payment Required";
    case StatusCode::Forbidden: return "Forbidden";
    case StatusCode::NotFound: return "Not Found";
    case StatusCode::MethodNotAllowed: return "Method Not Allowed";
    case StatusCode::NotAcceptable: return "Not Acceptable";
    case StatusCode::ProxyAuthenticationRequired: return "Proxy Authentication Required";
    case StatusCode::RequestTimeout: return "Request Timeout";
    case StatusCode::Conflict: return "Conflict";
    case StatusCode::Gone: return "Gone";
    case StatusCode::LengthRequired: return "Length Required";
    case StatusCode::PreconditionFailed: return "Precondition Failed";
    case StatusCode::RequestEntityTooLarge: return "Request Entity Too Large";
    case StatusCode::RequestURITooLong: return "Request-URI Too Long";
    case StatusCode::UnsupportedMediaType: return "Unsupported Media Type";
    case StatusCode::RequestedRangeNotSatisfiable: return "Requested Range Not Satisfiable";
    case StatusCode::ExpectationFailed: return "Expectation Failed";
    case StatusCode::InternalServerError: return "Internal Server Error";
    case StatusCode::NotImplemented: return "Not Implemented";
    case StatusCode::BadGateway: return "Bad Gateway";
    case StatusCode::ServiceUnavailable: return "Service Unavailable";
    case StatusCode::GatewayTimeout: return "Gateway Timeout";
    case StatusCode::HTTPVersionNotSupported: return "HTTP Version Not Supported";

    case StatusCode::Reserved: return "<reserved>";
    default: return "<unknown-status>";
    }
}

inline std::ostream& operator<<(std::ostream& os, StatusCode sc)
{
    return os << status_message(sc);
}

class Request
{
public:
    explicit Request(beast::http::request<beast::http::string_body> req)
    : m_req(std::move(req))
    {
    }

    std::string_view url() const noexcept
    {
        auto target = m_req.target();
        return std::string_view(target.data(), target.size());
    }

    std::string_view field(const char* hdr) const noexcept
    {
        auto stringRef = m_req[hdr];
        return std::string_view(stringRef.data(), stringRef.size());
    }

    std::string_view field(std::string_view hdr) const noexcept
    {
        auto stringRef = m_req[boost::string_view(hdr.data(), hdr.size())];
        return sv_cast<std::string_view>(stringRef);
    }

    std::string_view body() const noexcept
    {
        return sv_cast<std::string_view>(m_req.body());
    }

private:
    beast::http::request<beast::http::string_body> m_req;
};

struct Response
{
    Response() = default;
    explicit Response(StatusCode s);
    explicit Response(beast::http::status s);
    Response(StatusCode s, std::string b);
    Response(beast::http::status, std::string b);

    bool operator==(const Response& other) const noexcept;

    StatusCode status;
    std::string body;
};

}
}

namespace std
{

inline error_code make_error_code(upnp::http::error::ErrorCode e) noexcept
{
    return error_code(static_cast<int>(e), upnp::http::error::HttpErrorCategory::get());
}

template<>
struct is_error_code_enum<upnp::http::error::ErrorCode> : std::true_type
{
};

}
