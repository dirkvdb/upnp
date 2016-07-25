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
, m_ssdpClient(client.ioService())
, m_timer(client.ioService())
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
    
    m_self = std::shared_ptr<DeviceScanner>(this, [] (DeviceScanner*) {
        // empty deleter;
    });
}

void DeviceScanner::start()
{
    log::debug("Start device scanner, known devices ({})", m_devices.size());

    m_ssdpClient.run();
    m_timer.expires_from_now(s_timeCheckInterval);
    m_timer.async_wait([this] (const std::error_code& e) {
        if (e != asio::error::operation_aborted)
        {
            checkForDeviceTimeouts();
        }
    });
}

void DeviceScanner::stop()
{
    log::debug("Stop device scanner, known devices ({})", m_devices.size());
    m_timer.cancel();
    m_ssdpClient.stop();
}

void DeviceScanner::refresh()
{
    if (m_types.size() == 1)
    {
        m_ssdpClient.search(deviceTypeToString(*m_types.begin()));
    }
    else
    {
        m_ssdpClient.search();
    }
}

uint32_t DeviceScanner::getDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

std::shared_ptr<Device> DeviceScanner::getDevice(const std::string& udn) const
{
    std::promise<std::shared_ptr<Device>> p;
    auto fut = p.get_future();
    
    m_upnpClient.ioService().post([this, &udn, &p] {
        p.set_value(m_devices.at(udn));
    });
    
    return fut.get();
}

std::map<std::string, std::shared_ptr<Device>> DeviceScanner::getDevices() const
{
    std::promise<std::map<std::string, std::shared_ptr<Device>>> p;
    auto fut = p.get_future();
    
    m_upnpClient.ioService().post([this, &p] () {
        p.set_value(m_devices);
    });
    
    return fut.get();
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
    m_upnpClient.getFile(url, [cb, url] (Status status, std::string xml) {
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
            downloadDeviceXml(info.location, [device, exp = info.expirationTime] (const std::string& xml) {
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
    downloadDeviceXml(info.location, [location, exp = info.expirationTime, self = std::weak_ptr<DeviceScanner>(m_self)] (const std::string& xml) {
        try
        {
            auto selfPtr = self.lock();
            if (!selfPtr)
            {
                return;
            }
        
            auto device = std::make_shared<Device>();
            device->location = location;
            device->timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(exp);
            xml::parseDeviceInfo(xml, *device);

            auto iter = selfPtr->m_devices.find(device->udn);
            if (iter == selfPtr->m_devices.end())
            {
                log::info("Device added to the list: {} ({})", device->friendlyName, device->udn);
                selfPtr->m_devices.emplace(device->udn, device);
                selfPtr->DeviceDiscoveredEvent(device);
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
