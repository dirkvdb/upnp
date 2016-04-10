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

#include "http_parser.h"
#include <algorithm>

namespace upnp
{
namespace http
{

enum class Type
{
    Request = HTTP_REQUEST,
    Response = HTTP_RESPONSE,
    Both = HTTP_BOTH
};

enum class Method
{
    Notify = HTTP_NOTIFY,
    Search = HTTP_MSEARCH,
    Subscribe = HTTP_SUBSCRIBE,
    Unsubscribe = HTTP_UNSUBSCRIBE,
    Get = HTTP_GET,
    Head = HTTP_HEAD,
    Post = HTTP_POST
};

struct Header
{
    Header(std::string f)
    : field(std::move(f))
    {
    }

    std::string field;
    std::string value;
};

class Parser
{
public:
    Parser(Type type)
    : m_state(State::Initial)
    {
        m_settings.on_headers_complete = nullptr;
        m_settings.on_message_begin = [] (http_parser* parser) -> int {
            auto thisPtr = reinterpret_cast<Parser*>(parser->data);
            thisPtr->m_state = State::Initial;
            return 0;
        };
        m_settings.on_message_complete = nullptr;
        m_settings.on_url = nullptr;
        m_settings.on_status = nullptr;

        m_settings.on_header_value = [] (http_parser* parser, const char* str, size_t length) -> int {
            if (length > 0)
            {
                auto thisPtr = reinterpret_cast<Parser*>(parser->data);
                if (thisPtr->m_state == State::ParsingFieldValue)
                {
                    thisPtr->m_headers.back().value.append(str, length);
                }
                else
                {
                    thisPtr->m_headers.back().value.assign(str, length);
                }
                
                thisPtr->m_state = State::ParsingFieldValue;
            }
            
            return 0;
        };

        m_settings.on_header_field = [] (http_parser* parser, const char* str, size_t length) -> int {
            if (length > 0)
            {
                auto thisPtr = reinterpret_cast<Parser*>(parser->data);
                if (thisPtr->m_state == State::ParsingField)
                {
                    thisPtr->m_headers.back().field.append(str, length);
                }
                else
                {
                    thisPtr->m_headers.emplace_back(std::string(str, length));
                }
                
                thisPtr->m_state = State::ParsingField;
            }
            return 0;
        };

        m_settings.on_body = [] (http_parser* parser, const char* str, size_t length) -> int {
            if (length > 0)
            {
                auto thisPtr = reinterpret_cast<Parser*>(parser->data);
                if (thisPtr->m_state == State::ParsingBody)
                {
                    thisPtr->m_body.append(str, length);
                }
                else
                {
                    thisPtr->m_body.assign(str, length);
                }
                
                thisPtr->m_state = State::ParsingBody;
            }
            return 0;
        };

        m_parser.data = this;
        http_parser_init(&m_parser, static_cast<http_parser_type>(type));
    }

    void setHeadersCompletedCallback(std::function<void()> cb)
    {
        m_headersCompletedCb = std::move(cb);
        m_settings.on_headers_complete = [] (http_parser* parser) {
            auto thisPtr = reinterpret_cast<Parser*>(parser->data);
            thisPtr->m_headersCompletedCb();
            return 1;
        };
    }

    void setCompletedCallback(std::function<void()> cb)
    {
        m_completedCb = std::move(cb);
        m_settings.on_message_complete = [] (http_parser* parser) {
            auto thisPtr = reinterpret_cast<Parser*>(parser->data);
            thisPtr->m_completedCb();
            return 0;
        };
    }

    size_t parse(const char* data, size_t dataSize)
    {
        auto size = http_parser_execute(&m_parser, &m_settings, data, dataSize);
        if (m_parser.http_errno != HPE_OK)
        {
            throw std::runtime_error(fmt::format("Failed to parse http message: {}", http_errno_description(HTTP_PARSER_ERRNO(&m_parser))));
        }

        return size;
    }

    void parse(const std::string& data)
    {
        parse(data.data(), data.size());
    }

    const std::string& headerValue(const char* name)
    {
        auto iter = std::find_if(m_headers.begin(), m_headers.end(), [&] (const Header& hdr) {
            return strcasecmp(hdr.field.data(), name) == 0;
        });

        if (iter == m_headers.end())
        {
            throw std::runtime_error("Header not found: " + std::string(name));
        }

        return iter->value;
    }

    Method getMethod() const noexcept
    {
        return static_cast<Method>(m_parser.method);
    }

    uint32_t getStatus() const noexcept
    {
        return m_parser.status_code;
    }

    std::string stealBody() noexcept
    {
        return std::move(m_body);
    }

    const std::vector<Header>& headers() const
    {
        return m_headers;
    }

private:
    enum class State
    {
        Initial,
        ParsingField,
        ParsingFieldValue,
        ParsingBody
    };

    http_parser_settings m_settings;
    http_parser m_parser;

    State m_state;
    std::vector<Header> m_headers;
    std::string m_body;
    std::function<void()> m_completedCb;
    std::function<void()> m_headersCompletedCb;
};

}
}

