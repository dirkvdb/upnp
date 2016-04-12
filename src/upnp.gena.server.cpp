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
#include "upnp/upnptypes.h"
#include "upnp.http.parser.h"
#include "upnp.ssdp.parseutils.h"

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
    "HTTP/1.1 200 OK\r\n"
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
    void closeConnection(uv::socket::Tcp* client)
    {
        client->close([client] () {
            delete client;
        });
    }

    const std::string& required(const std::string& value)
    {
        if (value.empty())
        {
            throw std::runtime_error("Required value not present");
        }

        return value;
    }

    void writeResponse(uv::socket::Tcp* client, const std::string& response, bool closeConnectionAfterWrite)
    {
        client->write(uv::Buffer(response, uv::Buffer::Ownership::No), [client, closeConnectionAfterWrite] (int32_t status) {
            if (status != 0)
            {
                log::error("Failed to write response: {}", status);
            }

            if (closeConnectionAfterWrite)
            {
                closeConnection(client);
            }
        });
    }
}

Server::Server(uv::Loop& loop, const uv::Address& address, std::function<void(const SubscriptionEvent&)> cb)
: m_loop(loop)
, m_socket(loop)
, m_eventCb(std::move(cb))
{
    assert(m_eventCb);
    log::info("GENA: Start server on http://{}:{}", address.ip(), address.port());

    m_socket.bind(address);
    m_socket.listen(128, [this] (int32_t status) {
        if (status == 0)
        {
            log::info("GENA: Connection attempt");
            auto client = new uv::socket::Tcp(m_loop);
            auto parser = std::make_shared<http::Parser>(http::Type::Request);

            parser->setCompletedCallback([this, parser, client] () {
                try
                {
                    if (parser->getMethod() == http::Method::Notify)
                    {
                        m_currentEvent.sid = required(parser->headerValue("SID"));
                        m_currentEvent.sequence = std::stoi(required(parser->headerValue("SEQ")));
                        m_currentEvent.data = parser->stealBody();
                        if (parser->headerValue("NT") != "upnp:event")
                        {
                            throw Exception(ErrorCode::BadRequest);
                        }

                        if (parser->headerValue("NTS") != "upnp:propchange")
                        {
                            throw Exception(ErrorCode::BadRequest);
                        }

                        m_eventCb(m_currentEvent);

                        writeResponse(client, okResponse, parser->getFlags().isSet(http::Parser::Flag::ConnectionClose));
                    }
                    else
                    {
                        log::warn("GENA: Unexpected method type recieved: {}", http::Parser::methodToString(parser->getMethod()));
                    }
                }
                catch (Exception& e)
                {
                    auto body = fmt::format(g_body, e.getErrorCode(), e.what());
                    writeResponse(client, fmt::format(errorResponse, body.size(), body), parser->getFlags().isSet(http::Parser::Flag::ConnectionClose));
                    delete client;
                }
                catch (std::exception& e)
                {
                    auto body = fmt::format(g_body, static_cast<int32_t>(ErrorCode::BadRequest), e.what());
                    writeResponse(client, fmt::format(errorResponse, body), parser->getFlags().isSet(http::Parser::Flag::ConnectionClose));
                    delete client;
                }
            });

            try
            {
                m_socket.accept(*client);
                client->read([=] (ssize_t nread, const uv::Buffer& data) {
                    if (nread < 0)
                    {
                        if (nread != UV_EOF)
                        {
                            log::error("Failed to read from socket");
                        }
                        else
                        {
                            log::info("End of stream");
                        }
                    }
                    else
                    {
                        try
                        {
                            parser->parse(data.data(), nread);
                        }
                        catch (std::exception& e)
                        {
                            log::error("Failed to parse request: {}", e.what());
                        }
                    }
                });
            }
            catch (std::exception& e)
            {
                log::error("GENA: Failed to accept connection: {}", e.what());
                client->close(nullptr);
            }
        }
        else
        {
            log::error("GENA: Failed to listen on socket: {}", status);
        }
    });
}

Server::~Server() noexcept
{
    // Stop has to be called before destruction
    assert(m_socket.isClosing());
}

void Server::stop(std::function<void()> cb)
{
    m_socket.close(cb);
}

uv::Address Server::getAddress() const
{
    auto addr = m_socket.getSocketName();
    log::debug("GENA address: {} {}", addr.ip(), addr.port());
    return addr;
}

}
}
