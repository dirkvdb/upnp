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

namespace upnp
{
namespace gena
{

using namespace utils;

Server::Server(uv::Loop& loop, const uv::Address& address)
: m_loop(loop)
, m_socket(loop)
{
    log::info("GENA: Start server on http://{}:{}", address.ip(), address.port());
    
    m_socket.bind(address);
    m_socket.listen(128, [this] (int32_t status) {
        if (status == 0)
        {
            log::info("GENA: Connection attempt");
            auto client = std::make_unique<uv::socket::Tcp>(m_loop);
            
            try
            {
                m_socket.accept(*client);
                client->read([=] (ssize_t nread, uv::Buffer data) {
                    if (nread < 0)
                    {
                        if (nread != UV_EOF)
                        {
                            log::error("Failed to read from socket");
                        }
                    }
                    else
                    {
                        handleEvent(data);
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

void Server::handleEvent(const uv::Buffer& data)
{
    log::info("GENA: Event -> {}", std::string(data.data(), data.size()));
}

}
}