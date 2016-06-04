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

#include "upnp/upnp.devicescanner.h"
#include "upnp/upnp.xml.parseutils.h"
#include "upnp/upnp.clientinterface.h"

#include "utils/log.h"

#include <chrono>
#include <algorithm>

namespace upnp
{

using namespace utils;
using namespace std::placeholders;
using namespace std::chrono_literals;

static const auto s_timeCheckInterval = 60s;

DeviceScanner::DeviceScanner(IClient& client, DeviceType type)
: DeviceScanner(client, std::set<DeviceType>{ type })
{
}

DeviceScanner::DeviceScanner(IClient& client, std::set<DeviceType> types)
: m_upnpClient(client)
, m_ssdpClient(client.loop())
, m_timer(client.loop())
, m_types(types)
{
    m_ssdpClient.setDeviceNotificationCallback([=] (const ssdp::DeviceNotificationInfo& info) {
        if (info.type == ssdp::NotificationType::Alive)
        {
            onDeviceDiscovered(info);
        }
        else if (info.type == ssdp::NotificationType::ByeBye)
        {
            onDeviceDissapeared(info);
        }
    });
}

void DeviceScanner::start()
{
    log::debug("Start device scanner, known devices ({})", m_devices.size());

    uv::asyncSend(m_upnpClient.loop(), [this] () {
        m_ssdpClient.run();
        m_timer.start(s_timeCheckInterval, s_timeCheckInterval, [this] () {
            checkForDeviceTimeouts();
        });
    });
}

void DeviceScanner::stop(std::function<void()> cb)
{
    uv::asyncSend(m_upnpClient.loop(), [this, cb] () {
        log::debug("Stop device scanner, known devices ({})", m_devices.size());
        m_timer.stop();
        m_ssdpClient.stop(cb);
    });
}

void DeviceScanner::refresh()
{
    uv::asyncSend(m_upnpClient.loop(), [this] () {
        if (m_types.size() == 1)
        {
            m_ssdpClient.search(deviceTypeToString(*m_types.begin()));
        }
        else
        {
            m_ssdpClient.search();
        }
    });
}

uint32_t DeviceScanner::getDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

std::shared_ptr<Device> DeviceScanner::getDevice(const std::string& udn) const
{
    return uv::asyncSend<std::shared_ptr<Device>>(m_upnpClient.loop(), [this, &udn] () {
        return m_devices.at(udn);
    }).get();
}

std::map<std::string, std::shared_ptr<Device>> DeviceScanner::getDevices() const
{
    return uv::asyncSend<decltype(m_devices)>(m_upnpClient.loop(), [this] () {
        return m_devices;
    }).get();
}

void DeviceScanner::checkForDeviceTimeouts()
{
    auto now = std::chrono::system_clock::now();
    auto mapEnd = m_devices.end();
    for (auto iter = m_devices.begin(); iter != mapEnd;)
    {
        if (now > iter->second->timeoutTime)
        {
            auto dev = iter->second;
            log::info("Device timed out removing it from the list: {}", iter->second->friendlyName);
            iter = m_devices.erase(iter);
            DeviceDissapearedEvent(dev);
        }
        else
        {
            ++iter;
        }
    }
}

void DeviceScanner::downloadDeviceXml(const std::string& url, std::function<void(std::string)> cb)
{
    m_upnpClient.getFile(url, [=] (Status status, std::string xml) {
        if (!status)
        {
            log::error("Failed to download device info {} ({})", url, status.what());
            return;
        }

        cb(std::move(xml));
    });
}

void DeviceScanner::onDeviceDiscovered(const ssdp::DeviceNotificationInfo& info)
{
    auto deviceType = deviceTypeFromString(info.deviceType);
    auto typeIter = m_types.find(deviceType);
    if (typeIter == m_types.end() || deviceType.version < typeIter->version)
    {
        return;
    }

    auto iter = m_devices.find(info.deviceId);
    if (iter != m_devices.end())
    {
        auto device = iter->second;

        // device already known, just update the timeout time
        device->timeoutTime =  std::chrono::system_clock::now() + std::chrono::seconds(info.expirationTime);

        // check if the location is still the same (perhaps a new ip or port)
        if (device->location != info.location)
        {
            // update the device, ip or port has changed
            log::debug("Update device, location has changed: {} -> {}", device->location, info.location);
            downloadDeviceXml(info.location, [this, device, exp = info.expirationTime] (const std::string& xml) {
                try
                {
                    device->timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(exp);
                    xml::parseDeviceInfo(xml, *device);
                }
                catch (std::exception& e)
                {
                    log::error(e.what());
                }
            });
        }

        return;
    }

    log::debug("Device discovered: {} {}", info.serviceType, info.location);

    auto location = info.location;
    log::debug("download xml: {}", location);
    downloadDeviceXml(info.location, [this, location, exp = info.expirationTime] (const std::string& xml) {
        try
        {
            auto device = std::make_shared<Device>();
            device->location = location;
            device->timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(exp);
            xml::parseDeviceInfo(xml, *device);

            auto iter = m_devices.find(device->udn);
            if (iter == m_devices.end())
            {
                log::info("Device added to the list: {} ({})", device->friendlyName, device->udn);
                m_devices.emplace(device->udn, device);
                DeviceDiscoveredEvent(device);
            }
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    });
}

void DeviceScanner::onDeviceDissapeared(const ssdp::DeviceNotificationInfo& info)
{
    auto iter = m_devices.find(info.deviceId);
    if (iter != m_devices.end())
    {
        DeviceDissapearedEvent(iter->second);
        m_devices.erase(iter);
    }
}

}
