//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include <chrono>
#include <cinttypes>
#include <unordered_map>
#include <asio.hpp>

#include "upnp/upnp.uv.h"
#include "upnp/upnp.types.h"
#include "upnp/upnp.ssdp.server.h"
#include "upnp/upnp.rootdeviceinterface.h"

namespace upnp
{

namespace http
{
    class Parser;
    class Server;
    class Client;
}

struct Device;

class RootDevice : public IRootDevice
{
public:
    RootDevice(std::chrono::seconds advertiseInterval);
    ~RootDevice() noexcept;

    void initialize() override {}
    void initialize(const std::string& interfaceName);
    void uninitialize() override;

    std::string getWebrootUrl() override;
    void registerDevice(const std::string& deviceDescriptionXml, const Device& dev) override;

    std::string getUniqueDeviceName() override;

    void addFileToHttpServer(const std::string& path, const std::string& contentType, const std::string& data) override;
    void addFileToHttpServer(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data) override;
    void removeFileFromHttpServer(const std::string& path) override;

    void notifyEvent(const std::string& serviceId, std::string eventData) override;

    uv::Loop& loop() noexcept override;

private:
    std::string onSubscriptionRequest(http::Parser& parser) noexcept;
    std::string onUnsubscriptionRequest(http::Parser& parser) noexcept;
    std::string onActionRequest(http::Parser& parser);

    uv::Loop                        m_loop;
    asio::io_service                m_io;
    std::unique_ptr<http::Server>   m_httpServer;
    std::unique_ptr<http::Client>   m_httpClient;
    std::unique_ptr<ssdp::Server>   m_ssdpServer;

    Device                          m_device;
    std::chrono::seconds            m_advertiseInterval;

    std::unique_ptr<std::thread>    m_thread;
    std::unique_ptr<std::thread>    m_asioThread;

    struct SubscriptionData
    {
        std::string deliveryUrl;
        std::chrono::steady_clock::time_point expirationTime;
        uint32_t sequence = 0;
    };

    std::unordered_map<std::string, SubscriptionData> m_subscriptions;
};

}
