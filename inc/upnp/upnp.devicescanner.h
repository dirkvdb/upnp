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

#include <map>
#include <set>
#include <string>
#include <mutex>
#include <memory>
#include <future>
#include <thread>

#include "upnp/upnp.device.h"
#include "upnp/upnp.ssdp.client.h"
#include "upnp/upnp.http.client.h"

#include "utils/signal.h"

namespace upnp
{

class IClient;

class DeviceScanner
{
public:
    DeviceScanner(IClient& client, DeviceType type);
    DeviceScanner(IClient& client, std::set<DeviceType> types);

    void start();
    void stop(std::function<void()> cb);
    void refresh();

    uint32_t getDeviceCount() const;
    std::shared_ptr<Device> getDevice(const std::string& udn) const;
    std::map<std::string, std::shared_ptr<Device>> getDevices() const;

    utils::Signal<std::shared_ptr<Device>> DeviceDiscoveredEvent;
    utils::Signal<std::shared_ptr<Device>> DeviceDissapearedEvent;

private:
    void onDeviceDiscovered(const ssdp::DeviceNotificationInfo& info);
    void onDeviceDissapeared(const ssdp::DeviceNotificationInfo& info);
    void downloadDeviceXml(const std::string& url, std::function<void(std::string)>);
    void checkForDeviceTimeouts();

    IClient&                                        m_upnpClient;
    ssdp::Client                                    m_ssdpClient;
    uv::Timer                                       m_timer;
    const std::set<DeviceType>                      m_types;
    std::map<std::string, std::shared_ptr<Device>>  m_devices;
};

}
