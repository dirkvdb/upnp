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

#include "upnp/upnp.http.server.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <map>
#include <mutex>
#include <memory>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/fileoperations.h"

#include "upnp/upnp.uv.h"
#include "upnp.http.parser.h"

using namespace utils;
using namespace std::literals::chrono_literals;

namespace upnp
{
namespace http
{

static const std::string s_errorResponse =
    "HTTP/1.1 {} {}\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "\r\n";

static const std::string s_response =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: {}\r\n"
    "\r\n";

Server::Server(uv::Loop& loop, const uv::Address& address)
: m_loop(loop)
, m_socket(loop)
{
    log::info("Start HTTP server on http://{}:{}", address.ip(), address.port());

    m_socket.bind(address);
    m_socket.listen(128, [this] (int32_t status) {
        if (status == 0)
        {
            log::info("Http server: Connection attempt");
            auto client = std::make_unique<uv::socket::Tcp>(m_loop);
            auto parser = std::make_shared<http::Parser>(http::Type::Request);

            auto clientPtr = client.get();
            m_clients.emplace(clientPtr, std::move(client));

            parser->setCompletedCallback([this, parser, client = clientPtr] () {
                bool closeConnection = parser->getFlags().isSet(http::Parser::Flag::ConnectionClose);
            
                try
                {
                    log::info("requested file: {}", parser->getUrl());
                    auto& file = m_serverdFiles.at(parser->getUrl());
                    if (parser->getMethod() == Method::Head)
                    {
                        writeResponse(client, fmt::format(s_response, file.data.size(), file.contentType), "", closeConnection);
                    }
                    else if (parser->getMethod() == Method::Get)
                    {
                        writeResponse(client, fmt::format(s_response, file.data.size(), file.contentType), file.data, closeConnection);
                    }
                    else
                    {
                        writeResponse(client, fmt::format(s_errorResponse, 501, "Not Implemented"), "", closeConnection);
                    }
                }
                catch (std::out_of_range&)
                {
                    writeResponse(client, fmt::format(s_errorResponse, 404, "Not Found"), "", closeConnection);
                }
                catch (std::exception& e)
                {
                    writeResponse(client, fmt::format(s_errorResponse, 500, "Internal Server Error"), "", closeConnection);
                }
            });

            try
            {
                m_socket.accept(*clientPtr);
                clientPtr->read([this, clientPtr, parser] (ssize_t nread, const uv::Buffer& data) {
                    if (nread < 0)
                    {
                        if (nread != UV_EOF)
                        {
                            log::error("Failed to read from socket {}", uv::getErrorString(static_cast<int32_t>(nread)));
                        }
                        else
                        {
                            log::debug("Conection closed by client");
                            cleanupClient(clientPtr);
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
                            cleanupClient(clientPtr);
                        }
                    }
                });
            }
            catch (std::exception& e)
            {
                log::error("HTTP server: Failed to accept connection: {}", e.what());
                cleanupClient(clientPtr);
            }
        }
        else
        {
            log::error("HTTP server: Failed to listen on socket: {}", status);
        }
    });
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents)
{
    m_serverdFiles.emplace(urlPath, HostedFile{contentType, contents});
}

std::string Server::getWebRootUrl() const
{
    auto addr = m_socket.getSocketName();
    return fmt::format("http://{}:{}", addr.ip(), addr.port());
}

void Server::writeResponse(uv::socket::Tcp* client, const std::string& header, const std::string& body, bool closeConnection)
{
    client->write(uv::Buffer(header, uv::Buffer::Ownership::No), [this, client, closeConnection, &body] (int32_t status) {
        if (status != 0)
        {
            log::error("Failed to write response: {}", status);
            
            if (closeConnection)
            {
                cleanupClient(client);
            }
        }
        else
        {
            client->write(uv::Buffer(body, uv::Buffer::Ownership::No), [this, client, closeConnection] (int32_t status) {
                if (status != 0)
                {
                    log::error("Failed to write response: {}", status);
                }
            
                if (closeConnection)
                {
                    cleanupClient(client);
                }
            });
        }
    });
}

void Server::cleanupClient(uv::socket::Tcp* client) noexcept
{
    client->close([=] () {
        m_clients.erase(client);
    });
}

}
}
