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

using namespace utils;
using namespace std::literals::chrono_literals;

namespace upnp
{
namespace http
{

Server::Server(uv::Loop& loop, int32_t port)
: m_timer(loop)
, m_connection(nullptr)
{
    mg_mgr_init(&m_mgr, this);
    m_connection = mg_bind(&m_mgr, std::to_string(port).c_str(), &Server::eventHandler);
    m_connection->user_data = this;

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(m_connection);
    memset(&m_serverOptions, 0, sizeof(m_serverOptions));
    m_serverOptions.document_root = "/DOESNOTEXIST";  // don't serve files on disk
    m_serverOptions.enable_directory_listing = "no";

    m_timer.start(1s, 1s, [=] () {
        mg_mgr_poll(&m_mgr, 1);
    });
}

Server::~Server()
{
    m_timer.stop();
    mg_mgr_free(&m_mgr);
}

void Server::addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents)
{
    m_serverdFiles.emplace(urlPath, HostedFile{contentType, contents});
}

std::string Server::getWebRootUrl() const
{
    auto addr = uv::Address::createIp4(m_connection->sa.sin);
    return fmt::format("http://{}:{}/", addr.ip(), addr.port());
}

void Server::eventHandler(mg_connection* conn, int event, void* eventData)
{
    http_message* message = reinterpret_cast<http_message*>(eventData);
    auto* thisPtr = reinterpret_cast<Server*>(conn->user_data);

    switch (event)
    {
    case MG_EV_HTTP_REQUEST:
        log::info("HTTP request: {}", std::string(message->uri.p, message->uri.len));
        try
        {
            auto& file = thisPtr->m_serverdFiles.at(std::string(message->uri.p, message->uri.len));
            mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                            "Content-type: %s\r\n"
                            "Content-length: %lu\r\n"
                            "\r\n%s", file.contentType.c_str(), file.data.size(), file.data.c_str());
        }
        catch (std::exception&)
        {
            // Serve static content
            mg_serve_http(conn, message, thisPtr->m_serverOptions);
        }
        break;
    default:
      break;
    }
}

}
}
