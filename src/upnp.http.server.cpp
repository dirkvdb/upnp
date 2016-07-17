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

#include "upnp.enumutils.h"
#include "stringview.h"

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
, m_sessionId(0)
{
}

void Server::start(const ip::tcp::endpoint& address)
{
    m_acceptor.open(address.protocol());
    m_acceptor.bind(address);
    m_acceptor.listen();
    accept();

    log::info("HTTP server listening on http://{}:{}", m_acceptor.local_endpoint().address(), m_acceptor.local_endpoint().port());
}

void Server::stop()
{
    m_acceptor.close();
}

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(io_service& io, const Server& srv, uint64_t id)
    : parser(http::Type::Request)
    , m_sessionId(id)
    , socket(io)
    , server(srv)
    {
        parser.setCompletedCallback([this] () { onHttpParseCompleted(); });
    }

    void receiveData()
    {
        socket.async_receive(buffer(buf), [this, self = shared_from_this()] (const std::error_code& error, size_t bytesReceived) {
            if (error)
            {
                if (error.value() == asio::error::eof)
                {
                    log::debug("[{}] Conection closed by client", m_sessionId);
                }
                else
                {
                    log::error("[{}] Failed to read from socket {}", m_sessionId, error.message());
                }

                socket.close();
                return;
            }

            try
            {
                //log::info("[{}] {}", m_sessionId, std::string_view(buf.data(), bytesReceived).data());
                parser.parse(buf.data(), bytesReceived);
                if (!parser.isCompleted())
                {
                    log::debug("[{}] Parse not completed", m_sessionId);
                    receiveData();
                }
                else
                {
                    log::debug("[{}] Parse completed", m_sessionId);
                }
            }
            catch (std::exception& e)
            {
                log::error("[{}] Failed to parse request: {}", m_sessionId, e.what());
                socket.close();
            }
        });
    }

    void writeResponse(std::shared_ptr<std::string> response, bool closeConnection)
    {
        async_write(socket, buffer(*response), [this, closeConnection, response, self = shared_from_this()] (const std::error_code& error, size_t) {
            if (error)
            {
                log::error("[{}] Failed to write response: {}", m_sessionId, error.message());
            }

            log::error("[{}] Wrote response: {}", m_sessionId, *response);
            if (error || closeConnection)
            {
                socket.close();
                log::error("[{}] Connection closed", m_sessionId);
            }
        });
    }

    void writeResponse(std::shared_ptr<std::string> header, asio::const_buffer body, bool closeConnection)
    {
        std::vector<asio::const_buffer> data { buffer(*header), body };

        async_write(socket, data, [this, closeConnection, header, thisPtr = shared_from_this()] (const std::error_code& error, size_t) {
            if (error)
            {
                log::error("[{}] Failed to write response: {}", m_sessionId, error.message());
            }

            log::error("[{}] Wrote response: {}", m_sessionId, *header);
            if (error || closeConnection)
            {
                socket.close();
                log::error("[{}] Connection closed", m_sessionId);
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
                log::info("[{}] requested file size: {}", m_sessionId, parser.getUrl());
                auto& file = server.m_servedFiles.at(parser.getUrl());
                writeResponse(std::make_shared<std::string>(fmt::format(s_response, 200, file.data.size(), file.contentType)), closeConnection);
            }
            else if (parser.getMethod() == Method::Get)
            {
                log::info("[{}] requested file: {}", m_sessionId, parser.getUrl());
                auto& file = server.m_servedFiles.at(parser.getUrl());

                auto rangeHeader = parser.headerValue("Range");
                if (rangeHeader.empty())
                {
                    writeResponse(std::make_shared<std::string>(fmt::format(s_response, 200, file.data.size(), file.contentType)), buffer(file.data), closeConnection);
                }
                else
                {
                    auto range = Parser::parseRange(rangeHeader);
                    auto size = (range.end == 0) ? (file.data.size() - range.start) : (range.end - range.start) + 1;
                    writeResponse(std::make_shared<std::string>(fmt::format(s_response, 206, size, file.contentType)), buffer(&file.data[range.start], size), closeConnection);
                }
            }
            else
            {
                auto& func = server.m_handlers.at(enum_value(parser.getMethod()));
                if (func)
                {
                    writeResponse(std::make_shared<std::string>(func(parser)), closeConnection);
                }
                else
                {
                    writeResponse(std::make_shared<std::string>(fmt::format(s_errorResponse, 501, "Not Implemented")), closeConnection);
                }
            }
        }
        catch (std::out_of_range&)
        {
            writeResponse(std::make_shared<std::string>(fmt::format(s_errorResponse, 404, "Not Found")), closeConnection);
        }
        catch (std::exception& e)
        {
            log::error("[{}] Http server: {}", m_sessionId, e.what());
            writeResponse(std::make_shared<std::string>(fmt::format(s_errorResponse, 500, "Internal Server Error")), closeConnection);
        }
    }

    Parser parser;
    uint64_t m_sessionId;
    ip::tcp::socket socket;
    std::array<char, 1024*10> buf;
    const Server& server;
};

void Server::accept()
{
    auto session = std::make_shared<Session>(m_io, *this, ++m_sessionId);
    log::info("Accepting session: {}", m_sessionId);
    m_acceptor.async_accept(session->socket, [this, session] (const asio::error_code& error) {
        if (error)
        {
            if (error.value() != asio::error::operation_aborted)
            {
                log::error("HTTP server: Failed to accept on socket: {}", error.message());
            }
            else
            {
                log::info("-----------    Aborted    ---------------");
            }
        }
        else
        {
            log::info("Http server: Connection attempt {}", m_sessionId);
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
