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

#include "upnp/upnp.asio.h"
#include "upnp/upnp.http.parseutils.h"
#include "upnp.enumutils.h"
#include "upnp/stringview.h"
#include "URI.h"

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>

using namespace utils;
using namespace asio;
using namespace std::literals::chrono_literals;

//#define DEBUG_HTTP

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
    m_acceptor.set_option(asio::socket_base::reuse_address(true));
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
    : m_sessionId(id)
    , socket(io)
    , server(srv)
    {
    }

    void receiveData()
    {
        beast::http::async_read(socket, m_buffer, m_request, [this, self = shared_from_this()] (const asio_error_code& error, size_t) {
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

            onHttpParseCompleted();
        });
    }

    void writeResponse(std::shared_ptr<std::string> response, bool closeConnection)
    {
#ifdef DEBUG_HTTP
        log::info("Write response: {}", *response);
#endif

        async_write(socket, buffer(*response), [this, closeConnection, response, self = shared_from_this()] (const asio_error_code& error, size_t) {
            if (error)
            {
                log::error("[{}] Failed to write response: {}", m_sessionId, error.message());
            }

            if (error || closeConnection)
            {
                socket.close();
                log::debug("[{}] Connection closed", m_sessionId);
            }
        });
    }

    void writeResponse(std::shared_ptr<std::string> header, asio::const_buffer body, bool closeConnection)
    {
        std::vector<asio::const_buffer> data { buffer(*header), body };

        async_write(socket, data, [this, closeConnection, header, thisPtr = shared_from_this()] (const asio_error_code& error, size_t) {
            if (error)
            {
                log::error("[{}] Failed to write response: {}", m_sessionId, error.message());
            }

            if (error || closeConnection)
            {
                socket.close();
                log::debug("[{}] Connection closed", m_sessionId);
            }
        });
    }

    void onHttpParseCompleted()
    {
        bool closeConnection = m_request["Connection"] == "close";

        try
        {
            auto methodStr = m_request.method_string();
            auto method = methodFromString(std::string_view(methodStr.data(), methodStr.size()));

            auto& func = server.m_handlers.at(enum_value(method));
            if (func)
            {
                log::debug("[{}] handle request: {} {}", m_sessionId, m_request.method_string(), m_request.target());
                writeResponse(std::make_shared<std::string>(func(Request(m_request))), closeConnection);
                return;
            }

            auto& asyFunc = server.m_asyncHandlers.at(enum_value(method));
            if (asyFunc)
            {
                log::debug("[{}] handle request: {} {}", m_sessionId, m_request.method_string(), m_request.target());
                auto self = shared_from_this();
                if (asyFunc(Request(m_request), [=] (std::string response) {
                    self->writeResponse(std::make_shared<std::string>(std::move(response)), closeConnection);
                }))
                {
                    // Request handled
                    return;
                }
            }

            if (method == http::Method::Head)
            {
                log::info("[{}] requested file size: {}", m_sessionId, m_request.target());
                auto target = m_request.target();
                auto& file = server.m_servedFiles.at(std::string(target.begin(), target.end()));
                return writeResponse(std::make_shared<std::string>(fmt::format(s_response, 200, file.data.size(), file.contentType)), closeConnection);
            }
            else if (method == http::Method::Get)
            {
                log::info("[{}] requested file: {}", m_sessionId, m_request.target());
                auto target = m_request.target();
                auto& file = server.m_servedFiles.at(std::string(target.begin(), target.end()));
                auto rangeHeader = m_request["Range"];
                if (rangeHeader.empty())
                {
                    return writeResponse(std::make_shared<std::string>(fmt::format(s_response, 200, file.data.size(), file.contentType)), buffer(file.data), closeConnection);
                }
                else
                {
                    auto range = http::parseutils::parseRange(rangeHeader.to_string());
                    auto size = (range.end == 0) ? (file.data.size() - range.start) : (range.end - range.start) + 1;
                    return writeResponse(std::make_shared<std::string>(fmt::format(s_response, 206, size, file.contentType)), buffer(&file.data[range.start], size), closeConnection);
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

    beast::http::request<beast::http::string_body> m_request;
    beast::flat_buffer m_buffer;
    uint64_t m_sessionId;
    ip::tcp::socket socket;
    const Server& server;
};

void Server::accept()
{
    auto session = std::make_shared<Session>(m_io, *this, ++m_sessionId);
#ifdef DEBUG_HTTP
    log::debug("Accepting session: {}", m_sessionId);
#endif
    m_acceptor.async_accept(session->socket, [this, session] (const asio_error_code& error) {
        if (error)
        {
            if (error.value() != asio::error::operation_aborted)
            {
                log::error("HTTP server: Failed to accept on socket: {}", error.message());
            }
        }
        else
        {
#ifdef DEBUG_HTTP
            log::info("Http server: Connection attempt {}", m_sessionId);
#endif
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
    if (addr.address() != ip::address_v4::any())
    {
        return fmt::format("http://{}:{}", addr.address(), addr.port());
    }

    // Use the first non localhost interface
    auto intfs = getNetworkInterfaces();
    for (auto& i : intfs)
    {
        if (!i.isLoopback && i.address.is_v4())
        {
            return fmt::format("http://{}:{}", i.address, addr.port());
        }
    }

    // no non localhost interface found, return the first entry
    for (auto& i : intfs)
    {
        if (i.address.is_v4())
        {
            return fmt::format("http://{}:{}", intfs.front().address, addr.port());
        }
    }

    throw std::runtime_error("Interface information could not be obtained");
}

asio::ip::tcp::endpoint Server::getAddress() const
{
    return m_acceptor.local_endpoint();
}

void Server::setRequestHandler(Method method, RequestCb cb)
{
    m_handlers.at(enum_value(method)) = cb;
}

void Server::setRequestHandler(Method method, AsyncRequestCb cb)
{
    m_asyncHandlers.at(enum_value(method)) = cb;
}

std::vector<std::pair<std::string, std::string>> Server::getQueryParameters(std::string_view url)
{
    return URI(std::string(url)).getQueryParameters();
}

}
}
