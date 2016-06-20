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

    m_acceptor.bind(address);
    accept();
}

void Server::stop()
{
    m_acceptor.close();
}

void Server::newConnection(std::shared_ptr<asio::ip::tcp::socket> socket)
{
    auto parser = std::make_shared<http::Parser>(http::Type::Request);
    
    parser->setCompletedCallback([this, parser, socket] () { onHttpParseCompleted(parser, socket); });

    auto buf = std::make_shared<std::array<char, 1024*10>>();
    socket->async_receive(buffer(*buf), [this, buf, socket, parser] (const std::error_code& error, size_t bytesReceived) {
        if (error)
        {
            log::error("Failed to read from socket {}", error.message());
            //log::debug("Conection closed by client");
            socket->close();
            return;
        }
        
        try
        {
            parser->parse(buf->data(), bytesReceived);
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse request: {}", e.what());
            socket->close();
        }
    });
}

void Server::accept()
{
    auto socket = std::make_shared<asio::ip::tcp::socket>(m_io);
    m_acceptor.async_accept(*socket, [this, socket] (const std::error_code& error) {
        if (error)
        {
            log::error("HTTP server: Failed to accept on socket: {}", error.message());
        }
        else
        {
            log::info("Http server: Connection attempt");
            newConnection(socket);
        }
        
        accept();
    });
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents)
{
    m_serverdFiles.emplace(urlPath, HostedFile{contentType, contents});
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::vector<uint8_t>& contents)
{
    m_serverdFiles.emplace(urlPath, HostedFile{contentType, std::string(contents.begin(), contents.end())});
}

void Server::removeFile(const std::string& urlPath)
{
    m_serverdFiles.erase(urlPath);
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

void Server::writeResponse(std::shared_ptr<asio::ip::tcp::socket> socket, const std::string& response, bool closeConnection)
{
    socket->async_send(buffer(response), [this, socket, closeConnection] (const std::error_code& error, size_t) {
        if (error)
        {
            log::error("Failed to write response: {}", error.message());
        }

        if (error || closeConnection)
        {
            socket->close();
        }
    });
}

void Server::writeResponse(std::shared_ptr<asio::ip::tcp::socket> socket, const std::string& header, asio::const_buffer body, bool closeConnection)
{
    std::vector<asio::const_buffer> data { buffer(header), body };
    
    socket->async_send(data, [this, socket, closeConnection] (const std::error_code& error, size_t) {
        if (error)
        {
            log::error("Failed to write response: {}", error.message());
        }
        
        if (error || closeConnection)
        {
            socket->close();
        }
    });
}

void Server::onHttpParseCompleted(std::shared_ptr<http::Parser> parser, std::shared_ptr<asio::ip::tcp::socket> socket)
{
    bool closeConnection = parser->getFlags().isSet(http::Parser::Flag::ConnectionClose);

    try
    {
        if (parser->getMethod() == Method::Head)
        {
            log::info("requested file size: {}", parser->getUrl());
            auto& file = m_serverdFiles.at(parser->getUrl());
            writeResponse(socket, fmt::format(s_response, 200, file.data.size(), file.contentType), closeConnection);
        }
        else if (parser->getMethod() == Method::Get)
        {
            log::info("requested file: {}", parser->getUrl());
            auto& file = m_serverdFiles.at(parser->getUrl());

            auto rangeHeader = parser->headerValue("Range");
            if (rangeHeader.empty())
            {
                writeResponse(socket, fmt::format(s_response, 200, file.data.size(), file.contentType), buffer(file.data), closeConnection);
            }
            else
            {
                auto range = Parser::parseRange(rangeHeader);
                auto size = (range.end == 0) ? (file.data.size() - range.start) : (range.end - range.start) + 1;
                writeResponse(socket, fmt::format(s_response, 206, size, file.contentType), buffer(&file.data[range.start], size), closeConnection);
            }
        }
        else
        {
            auto& func = m_handlers.at(enum_value(parser->getMethod()));
            if (func)
            {
                writeResponse(socket, func(*parser), closeConnection);
            }
            else
            {
                writeResponse(socket, fmt::format(s_errorResponse, 501, "Not Implemented"), closeConnection);
            }
        }
    }
    catch (std::out_of_range&)
    {
        writeResponse(socket, fmt::format(s_errorResponse, 404, "Not Found"), closeConnection);
    }
    catch (std::exception& e)
    {
        writeResponse(socket, fmt::format(s_errorResponse, 500, "Internal Server Error"), closeConnection);
    }
}

}
}
