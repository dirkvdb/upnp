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

#include "upnp.gena.server.h"

#include "utils/log.h"
#include "upnp.ssdp.parseutils.h"
#include "upnp/upnp.types.h"

namespace upnp
{
namespace gena
{

static const std::string okResponse =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: 41\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n"
    "<html><body><h1>200 OK</h1></body></html>";

static const std::string errorResponse =
    "HTTP/1.1 {} {}\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n"
    "{}";

static const std::string g_body = "<html><body><h1>{} {}</h1></body></html>";

using namespace utils;
using namespace std::placeholders;

namespace
{
    const std::string& required(const std::string& value)
    {
        if (value.empty())
        {
            throw std::runtime_error("Required value not present");
        }

        return value;
    }
}

Server::Server(asio::io_service& io, const asio::ip::tcp::endpoint& address, std::function<void(const SubscriptionEvent&)> cb)
: m_httpServer(io)
, m_eventCb(std::move(cb))
{
    assert(m_eventCb);
    log::info("GENA: Start server on http://{}:{}", address.address(), address.port());

    m_httpServer.setRequestHandler(http::Method::Notify, [this] (auto& parser) {
        try
        {
            m_currentEvent.sid = required(parser.headerValue("SID"));
            m_currentEvent.sequence = std::stoi(required(parser.headerValue("SEQ")));
            m_currentEvent.data = parser.stealBody();
            if (parser.headerValue("NT") != "upnp:event")
            {
                throw Status(ErrorCode::BadRequest);
            }

            if (parser.headerValue("NTS") != "upnp:propchange")
            {
                throw Status(ErrorCode::BadRequest);
            }

            m_eventCb(m_currentEvent);
            return okResponse;
        }
        catch (Status& e)
        {
            auto ec = errorCodeToInt(ErrorCode(e.getErrorCode()));
            auto body = fmt::format(g_body, ec, e.what());
            return fmt::format(errorResponse, ec, e.what(), body.size(), body);
        }
        catch (std::exception& e)
        {
            auto ec = errorCodeToInt(ErrorCode::BadRequest);
            auto body = fmt::format(g_body, ec, e.what());
            return fmt::format(errorResponse, ec, e.what(), body);
        }
    });

    m_httpServer.start(address);
}

void Server::stop()
{
    m_httpServer.stop();
}

asio::ip::tcp::endpoint Server::getAddress() const
{
    return m_httpServer.getAddress();
}

}
}
