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

#include "upnp/upnp.http.parser.h"
#include "upnp/upnp.flags.h"
#include "http_parser.h"
#include "utils/stringoperations.h"

#include "upnp.enumutils.h"

#include <algorithm>

namespace upnp
{

using namespace utils;

static const std::string g_emptyString;

static constexpr EnumMap<http::Type, http_parser_type> s_typeValues {{
    std::make_tuple(HTTP_REQUEST,    http::Type::Request),
    std::make_tuple(HTTP_RESPONSE,   http::Type::Response),
    std::make_tuple(HTTP_BOTH,       http::Type::Both),
}};

static constexpr EnumMap<http::Method, http_method> s_methodConv {{
    std::make_tuple(HTTP_NOTIFY,        http::Method::Notify),
    std::make_tuple(HTTP_MSEARCH,       http::Method::Search),

    std::make_tuple(HTTP_SUBSCRIBE,     http::Method::Subscribe),
    std::make_tuple(HTTP_UNSUBSCRIBE,   http::Method::Unsubscribe),
    
    std::make_tuple(HTTP_GET,           http::Method::Get),
    std::make_tuple(HTTP_HEAD,          http::Method::Head),
    std::make_tuple(HTTP_POST,          http::Method::Post),
}};

static constexpr EnumMapEndsWith<http_method, HTTP_UNLINK, http::Method> s_methodValues {{
    std::make_tuple(http::Method::Unknown,        HTTP_DELETE),
    std::make_tuple(http::Method::Get,            HTTP_GET),
    std::make_tuple(http::Method::Head,           HTTP_HEAD),
    std::make_tuple(http::Method::Post,           HTTP_POST),
    std::make_tuple(http::Method::Unknown,        HTTP_PUT),

    std::make_tuple(http::Method::Unknown,        HTTP_CONNECT),
    std::make_tuple(http::Method::Unknown,        HTTP_OPTIONS),
    std::make_tuple(http::Method::Unknown,        HTTP_TRACE),

    std::make_tuple(http::Method::Unknown,        HTTP_COPY),
    std::make_tuple(http::Method::Unknown,        HTTP_LOCK),
    std::make_tuple(http::Method::Unknown,        HTTP_MKCOL),
    std::make_tuple(http::Method::Unknown,        HTTP_MOVE),
    std::make_tuple(http::Method::Unknown,        HTTP_PROPFIND),
    std::make_tuple(http::Method::Unknown,        HTTP_PROPPATCH),
    std::make_tuple(http::Method::Unknown,        HTTP_SEARCH),
    std::make_tuple(http::Method::Unknown,        HTTP_UNLOCK),
    std::make_tuple(http::Method::Unknown,        HTTP_BIND),
    std::make_tuple(http::Method::Unknown,        HTTP_REBIND),
    std::make_tuple(http::Method::Unknown,        HTTP_UNBIND),
    std::make_tuple(http::Method::Unknown,        HTTP_ACL),

    std::make_tuple(http::Method::Unknown,        HTTP_REPORT),
    std::make_tuple(http::Method::Unknown,        HTTP_MKACTIVITY),
    std::make_tuple(http::Method::Unknown,        HTTP_CHECKOUT),
    std::make_tuple(http::Method::Unknown,        HTTP_MERGE),

    std::make_tuple(http::Method::Search,         HTTP_MSEARCH),
    std::make_tuple(http::Method::Notify,         HTTP_NOTIFY),
    std::make_tuple(http::Method::Subscribe,      HTTP_SUBSCRIBE),
    std::make_tuple(http::Method::Unsubscribe,    HTTP_UNSUBSCRIBE),

    std::make_tuple(http::Method::Unknown,        HTTP_PATCH),
    std::make_tuple(http::Method::Unknown,        HTTP_PURGE),

    std::make_tuple(http::Method::Unknown,        HTTP_MKCALENDAR),

    std::make_tuple(http::Method::Unknown,        HTTP_LINK),
    std::make_tuple(http::Method::Unknown,        HTTP_UNLINK),
}};


ADD_ENUM_MAP_TYPED(http::Type, http_parser_type, s_typeValues)
ADD_ENUM_MAP_TYPED(http::Method, http_method, s_methodConv)

namespace http
{

static_assert(enum_value(Parser::Flag::Chunked) == F_CHUNKED, "Flag mismatch");
static_assert(enum_value(Parser::Flag::KeepAlive) == F_CONNECTION_KEEP_ALIVE, "Flag mismatch");
static_assert(enum_value(Parser::Flag::ConnectionClose) == F_CONNECTION_CLOSE, "Flag mismatch");
static_assert(enum_value(Parser::Flag::ConnectionUpgrade) == F_CONNECTION_UPGRADE, "Flag mismatch");
static_assert(enum_value(Parser::Flag::Trailing) == F_TRAILING, "Flag mismatch");
static_assert(enum_value(Parser::Flag::Upgrade) == F_UPGRADE, "Flag mismatch");
static_assert(enum_value(Parser::Flag::SkipBody) == F_SKIPBODY, "Flag mismatch");
static_assert(enum_value(Parser::Flag::ContentLength) == F_CONTENTLENGTH, "Flag mismatch");

enum class State
{
    Initial,
    ParsingField,
    ParsingFieldValue,
    ParsingBody
};

struct Parser::Pimpl
{
    http_parser_settings settings;
    http_parser parser;
    
    Type type;
    State state = State::Initial;
    std::vector<Header> headers;
    std::string body;
    std::string url;
    std::function<void()> completedCb;
    std::function<void()> headersCompletedCb;
};

Parser::Parser(Type type)
: m_pimpl(std::make_unique<Pimpl>())
{
    m_pimpl->type = type;
    m_pimpl->settings.on_headers_complete = nullptr;
    m_pimpl->settings.on_message_begin = [] (http_parser* parser) -> int {
        auto thisPtr = reinterpret_cast<Parser*>(parser->data);
        thisPtr->clear();
        return 0;
    };
    m_pimpl->settings.on_message_complete = nullptr;
    m_pimpl->settings.on_status = nullptr;

    m_pimpl->settings.on_url = [] (http_parser* parser, const char* str, size_t length) -> int {
        auto thisPtr = reinterpret_cast<Parser*>(parser->data);
        thisPtr->m_pimpl->url.append(str, length);
        return 0;
    };

    m_pimpl->settings.on_header_value = [] (http_parser* parser, const char* str, size_t length) -> int {
        auto thisPtr = reinterpret_cast<Parser*>(parser->data);
        if (thisPtr->m_pimpl->state == State::ParsingFieldValue)
        {
            thisPtr->m_pimpl->headers.back().value.append(str, length);
        }
        else
        {
            thisPtr->m_pimpl->headers.back().value.assign(str, length);
        }

        thisPtr->m_pimpl->state = State::ParsingFieldValue;
        return 0;
    };

    m_pimpl->settings.on_header_field = [] (http_parser* parser, const char* str, size_t length) -> int {
        if (length > 0)
        {
            auto thisPtr = reinterpret_cast<Parser*>(parser->data);
            if (thisPtr->m_pimpl->state == State::ParsingField)
            {
                thisPtr->m_pimpl->headers.back().field.append(str, length);
            }
            else
            {
                thisPtr->m_pimpl->headers.emplace_back(std::string(str, length));
            }

            thisPtr->m_pimpl->state = State::ParsingField;
        }
        return 0;
    };

    m_pimpl->settings.on_body = [] (http_parser* parser, const char* str, size_t length) -> int {
        if (length > 0)
        {
            auto thisPtr = reinterpret_cast<Parser*>(parser->data);
            if (thisPtr->m_pimpl->state == State::ParsingBody)
            {
                thisPtr->m_pimpl->body.append(str, length);
            }
            else
            {
                thisPtr->m_pimpl->body.assign(str, length);
            }

            thisPtr->m_pimpl->state = State::ParsingBody;
        }
        return 0;
    };

    m_pimpl->parser.data = this;
    http_parser_init(&m_pimpl->parser, enum_typecast<http_parser_type>(type));
}

Parser::~Parser() = default;

void Parser::reset()
{
    http_parser_init(&m_pimpl->parser, enum_typecast<http_parser_type>(m_pimpl->type));
    clear();
}

void Parser::setHeadersCompletedCallback(std::function<void()> cb)
{
    m_pimpl->headersCompletedCb = std::move(cb);
    m_pimpl->settings.on_headers_complete = [] (http_parser* parser) {
        auto thisPtr = reinterpret_cast<Parser*>(parser->data);
        thisPtr->m_pimpl->headersCompletedCb();
        return 1;
    };
}

void Parser::setCompletedCallback(std::function<void()> cb)
{
    m_pimpl->completedCb = std::move(cb);
    m_pimpl->settings.on_message_complete = [] (http_parser* parser) {
        auto thisPtr = reinterpret_cast<Parser*>(parser->data);
        thisPtr->m_pimpl->completedCb();
        return 0;
    };
}

size_t Parser::parse(const char* data, size_t dataSize)
{
    auto size = http_parser_execute(&m_pimpl->parser, &m_pimpl->settings, data, dataSize);
    if (m_pimpl->parser.http_errno != HPE_OK)
    {
        throw std::runtime_error(fmt::format("Failed to parse http message: {}", http_errno_description(HTTP_PARSER_ERRNO(&m_pimpl->parser))));
    }

    return size;
}

size_t Parser::parse(const std::string& data)
{
    return parse(data.data(), data.size());
}

const std::string& Parser::headerValue(const char* name)
{
    auto iter = std::find_if(m_pimpl->headers.begin(), m_pimpl->headers.end(), [&] (const Header& hdr) {
        return strcasecmp(hdr.field.data(), name) == 0;
    });

    return iter == m_pimpl->headers.end() ? g_emptyString : iter->value;
}

Method Parser::getMethod() const noexcept
{
    return std::get<0>(s_methodValues[m_pimpl->parser.method]);
}

uint32_t Parser::getStatus() const noexcept
{
    return m_pimpl->parser.status_code;
}

std::string Parser::stealBody() noexcept
{
    return std::move(m_pimpl->body);
}

std::string Parser::getUrl() noexcept
{
    return m_pimpl->url;
}

const std::vector<Header>& Parser::headers() const
{
    return m_pimpl->headers;
}

Flags<Parser::Flag> Parser::getFlags() const noexcept
{
    return Flags<Flag>(m_pimpl->parser.flags);
}

const char* Parser::methodToString(Method m) noexcept
{
    return http_method_str(enum_typecast<http_method>(m));
}

void Parser::clear()
{
    m_pimpl->headers.clear();
    m_pimpl->body = std::string();
    m_pimpl->url.clear();
    m_pimpl->state = State::Initial;
}

Parser::Range Parser::parseRange(const std::string& range)
{
    Range result;
    
    if (!stringops::startsWith(range, "bytes="))
    {
        throw std::invalid_argument("Invalid range header: " + range);
    }

    auto split = stringops::tokenize(&range[6], '-');
    if (split.size() != 2)
    {
        throw std::invalid_argument("Invalid range header: " + range);
    }
    
    if (!split[0].empty())
    {
        result.start = std::stoul(split[0]);
    }
    
    if (!split[1].empty())
    {
        result.end = std::stoul(split[1]);
    }
    
    return result;
}

}
}

