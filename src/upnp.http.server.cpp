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
#include "upnp.enumutils.h"

using namespace utils;
using namespace asio;
using namespace std::literals::chrono_literals;

namespace upnp
{
namespace http
{

static const std::string s_errorResponse =
    "HTTP/1.1 {} {}\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "Content-Length: 0\r\n"
    "\r\n";

static const std::string s_response =
    "HTTP/1.1 {} OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: {}\r\n"
    "CONTENT-TYPE: {}\r\n"
    "\r\n";

Server::Server(asio::io_service& io)
: m_io(io)
, m_acceptor(io)
{
}

void Server::start(const ip::tcp::endpoint& address)
{
    log::info("Start HTTP server on http://{}:{}", address.address(), address.port());
    m_acceptor.open(address.protocol());
    m_acceptor.bind(address);
    accept();
}

void Server::stop()
{
    m_acceptor.close();
}

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(io_service& io, const Server& srv)
    : parser(http::Type::Request)
    , socket(io)
    , server(srv)
    {
        parser.setCompletedCallback([this] () { onHttpParseCompleted(); });
    }

    void receiveData()
    {
        socket.async_receive(buffer(buf), [this, thisPtr = shared_from_this()] (const std::error_code& error, size_t bytesReceived) {
            if (error)
            {
                log::error("Failed to read from socket {}", error.message());
                //log::debug("Conection closed by client");
                socket.close();
                return;
            }

            try
            {
                parser.parse(buf.data(), bytesReceived);
                if (!parser.isCompleted())
                {
                    receiveData();
                }
            }
            catch (std::exception& e)
            {
                log::error("Failed to parse request: {}", e.what());
                socket.close();
            }
        });
    }

    void writeResponse(const std::string& response, bool closeConnection)
    {
        socket.async_send(buffer(response), [this, closeConnection, thisPtr = shared_from_this()] (const std::error_code& error, size_t) {
            if (error)
            {
                log::error("Failed to write response: {}", error.message());
            }

            if (error || closeConnection)
            {
                socket.close();
            }
        });
    }

    void writeResponse(const std::string& header, asio::const_buffer body, bool closeConnection)
    {
        std::vector<asio::const_buffer> data { buffer(header), body };

        socket.async_send(data, [this, closeConnection, thisPtr = shared_from_this()] (const std::error_code& error, size_t) {
            if (error)
            {
                log::error("Failed to write response: {}", error.message());
            }

            if (error || closeConnection)
            {
                socket.close();
            }
        });
    }

    void onHttpParseCompleted()
    {
        bool closeConnection = parser.getFlags().isSet(http::Parser::Flag::ConnectionClose);

        try
        {
            if (parser.getMethod() == Method::Head)
            {
                log::info("requested file size: {}", parser.getUrl());
                auto& file = server.m_servedFiles.at(parser.getUrl());
                writeResponse(fmt::format(s_response, 200, file.data.size(), file.contentType), closeConnection);
            }
            else if (parser.getMethod() == Method::Get)
            {
                log::info("requested file: {}", parser.getUrl());
                auto& file = server.m_servedFiles.at(parser.getUrl());

                auto rangeHeader = parser.headerValue("Range");
                if (rangeHeader.empty())
                {
                    writeResponse(fmt::format(s_response, 200, file.data.size(), file.contentType), buffer(file.data), closeConnection);
                }
                else
                {
                    auto range = Parser::parseRange(rangeHeader);
                    auto size = (range.end == 0) ? (file.data.size() - range.start) : (range.end - range.start) + 1;
                    writeResponse(fmt::format(s_response, 206, size, file.contentType), buffer(&file.data[range.start], size), closeConnection);
                }
            }
            else
            {
                auto& func = server.m_handlers.at(enum_value(parser.getMethod()));
                if (func)
                {
                    writeResponse(func(parser), closeConnection);
                }
                else
                {
                    writeResponse(fmt::format(s_errorResponse, 501, "Not Implemented"), closeConnection);
                }
            }
        }
        catch (std::out_of_range&)
        {
            writeResponse(fmt::format(s_errorResponse, 404, "Not Found"), closeConnection);
        }
        catch (std::exception& e)
        {
            writeResponse(fmt::format(s_errorResponse, 500, "Internal Server Error"), closeConnection);
        }
    }

    Parser parser;
    ip::tcp::socket socket;
    std::array<char, 1024*10> buf;
    const Server& server;
};

void Server::accept()
{
    auto session = std::make_shared<Session>(m_io, *this);
    m_acceptor.async_accept(session->socket, [this, session] (const std::error_code& error) {
        if (error)
        {
            log::error("HTTP server: Failed to accept on socket: {}", error.message());
        }
        else
        {
            log::info("Http server: Connection attempt");
            session->receiveData();
            accept();
        }
    });
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents)
{
    m_servedFiles.emplace(urlPath, HostedFile{contentType, contents});
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::vector<uint8_t>& contents)
{
    m_servedFiles.emplace(urlPath, HostedFile{contentType, std::string(contents.begin(), contents.end())});
}

void Server::removeFile(const std::string& urlPath)
{
    m_servedFiles.erase(urlPath);
}

std::string Server::getWebRootUrl() const
{
    auto addr = m_acceptor.local_endpoint();
    return fmt::format("http://{}:{}", addr.address(), addr.port());
}

asio::ip::tcp::endpoint Server::getAddress() const
{
    return m_acceptor.local_endpoint();
}

void Server::setRequestHandler(Method method, RequestCb cb)
{
    m_handlers.at(enum_value(method)) = cb;
}

}
}
