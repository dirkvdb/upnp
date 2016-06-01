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

#include <cinttypes>

#include "upnp/upnp.types.h"
#include "upnp/upnp.http.server.h"
#include "upnp/upnp.ssdp.server.h"
#include "upnp/upnprootdeviceinterface.h"

namespace upnp
{

namespace http { class Parser; }

struct Device;

class RootDevice2 : public IRootDevice
{
public:
    RootDevice2(std::chrono::seconds advertiseInterval);
    ~RootDevice2() noexcept;

    void initialize() override {}
    void initialize(const std::string& interfaceName);
    void uninitialize() override;

    std::string getWebrootUrl();
    void registerDevice(const std::string& deviceDescriptionXml, const Device& dev);

    std::string getUniqueDeviceName() override;
    void acceptSubscription(const std::string&, const std::string&, const xml::Document&) override {}
    void notifyEvent(const std::string&, const xml::Document&) override {}

    void acceptSubscription(const std::string& serviceId, const std::string& subscriptionId, const std::string& response) override;
    void notifyEvent(const std::string& serviceId, const std::string& response) override;

private:
    std::string onSubscriptionRequest(http::Parser& parser);
    std::string onUnsubscriptionRequest(http::Parser& parser);
    std::string onActionRequest(http::Parser& parser);

    uv::Loop                        m_loop;
    std::unique_ptr<http::Server>   m_httpServer;
    std::unique_ptr<ssdp::Server>   m_ssdpServer;

    Device                          m_device;
    std::chrono::seconds            m_advertiseInterval;

    std::unique_ptr<std::thread>    m_thread;
};

}
