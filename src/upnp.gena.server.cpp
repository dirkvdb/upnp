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
#include "upnp.http.parser.h"

namespace upnp
{
namespace gena
{

static std::string okResponse =
"HTTP/1.1 200 OK\r\n"
"SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
"CONNECTION: close\r\n"
"CONTENT-LENGTH: 41\r\n"
"CONTENT-TYPE: text/html\r\n"
"\r\n"
"<html><body><h1>200 OK</h1></body></html>";


using namespace utils;
using namespace std::placeholders;

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
            auto client = std::make_unique<uv::socket::Tcp>(m_loop);
            auto parser = std::make_shared<http::Parser>(http::Type::Request);

            try
            {
                m_socket.accept(*client);
                client->read([=, cl = client.get()] (ssize_t nread, uv::Buffer data) {
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
                        parser->setCompletedCallback([&] () {
                            try
                            {
                                m_currentEvent.sid = parser->headerValue("SID");
                                m_currentEvent.sid = parser->headerValue("SEQ");
                                
                                assert(cl);
                                // return the response
                                cl->write(uv::Buffer(&okResponse[0], okResponse.size()), [] (int32_t status) {
                                    if (status != 0)
                                    {
                                        log::error("Failed to write response: {}", status);
                                    }
                                });
                            }
                            catch (std::exception& e)
                            {
                                log::error(e.what());
                                // TODO: return error response
                            }
                        });
                        
                        auto* dataPtr = data.data();
                        auto size = data.size();
                        
                        for (;;)
                        {
                            size_t nparsed = parser->parse(dataPtr, size);
                            if (nparsed == data.size())
                            {
                                return;
                            }

                            dataPtr += nparsed;
                            size -= nparsed;
                        }
                    }
                });

                m_clients.emplace_back(std::move(client));
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
    m_socket.close(nullptr);
}

uv::Address Server::getAddress() const
{
    auto addr = m_socket.getSocketName();
    log::debug("GENA address: {} {}", addr.ip(), addr.port());
    return addr;
}

}
}
