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

#include "utils/log.h"

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


class Parser
{
public:
    Parser(Type type)
    {
        m_settings.on_message_begin = [] (http_parser*) { return 0; };
        m_settings.on_message_complete = [] (http_parser*) { return 0; };

        m_settings.on_url = [] (http_parser*, const char* at, size_t) -> int {
            utils::log::debug("URL: {}", at);
            return 0;
        };

        m_settings.on_header_value = [] (http_parser*, const char* at, size_t) -> int {
            utils::log::debug("Value: {}", at);
            return 0;
        };

        m_settings.on_header_field = [] (http_parser*, const char* at, size_t) -> int {
            utils::log::debug("Field: {}", at);
            return 0;
        };

        http_parser_init(&m_parser, static_cast<http_parser_type>(type));
        m_parser.data = this;
    }

    void parse(const std::string& data)
    {
        http_parser_execute(&m_parser, &m_settings, data.c_str(), data.size());
        if (m_parser.http_errno)
        {
            throw std::runtime_error(fmt::format("Failed to parse http message: {}", http_errno_description(HTTP_PARSER_ERRNO(&m_parser))));
        }
    }

private:
    http_parser_settings    m_settings;
    http_parser             m_parser;
};

}
}

