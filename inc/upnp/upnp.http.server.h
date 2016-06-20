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
#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <asio.hpp>

#include "upnp/upnp.http.parser.h"

namespace upnp
{
namespace http
{

using RequestCb = std::function<std::string(http::Parser&)>;

class Server
{
public:
    Server(asio::io_service& io);

    void start(const asio::ip::tcp::endpoint& address);
    void stop();

    void addFile(const std::string& urlPath, const std::string& contentType, const std::string& contents);
    void addFile(const std::string& urlPath, const std::string& contentType, const std::vector<uint8_t>& contents);
    void removeFile(const std::string& urlPath);
    std::string getWebRootUrl() const;
    asio::ip::tcp::endpoint getAddress() const;

    void setRequestHandler(Method method, RequestCb cb);

private:
    struct HostedFile
    {
        std::string contentType;
        std::string data;
    };

    void newConnection(std::shared_ptr<asio::ip::tcp::socket> socket);
    void accept();
    void writeResponse(std::shared_ptr<asio::ip::tcp::socket> socket, const std::string& response, bool closeConnection);
    void writeResponse(std::shared_ptr<asio::ip::tcp::socket> socket, const std::string& header, asio::const_buffer body, bool closeConnection);

    void onHttpParseCompleted(std::shared_ptr<http::Parser> parser, std::shared_ptr<asio::ip::tcp::socket> socket);

    asio::io_service& m_io;
    asio::ip::tcp::acceptor m_acceptor;
    std::unordered_map<std::string, HostedFile> m_serverdFiles;

    std::array<RequestCb, std::underlying_type_t<Method>(Method::Unknown)> m_handlers;
};

}
}
