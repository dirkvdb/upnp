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
, m_types(std::move(types))
{
    // since everything is single threaded, this self reference indicates if this object
    // is destructed or not, so we can exit in the callback and avoid member access
    // This does imply that the destructor is invoked on the io service thread!

    m_self = std::shared_ptr<DeviceScanner>(this, [] (DeviceScanner*) {
        // empty deleter;
    });

    m_ssdpClient.setDeviceNotificationCallback([self = std::weak_ptr<DeviceScanner>(m_self)] (const ssdp::DeviceNotificationInfo& info) {
        auto selfPtr = self.lock();
        if (!selfPtr)
        {
            return;
        }

        if (info.type == ssdp::NotificationType::Alive)
        {
            selfPtr->onDeviceDiscovered(info);
        }
        else if (info.type == ssdp::NotificationType::ByeBye)
        {
            selfPtr->onDeviceDissapeared(info);
        }
    });
}

void DeviceScanner::start()
{
    log::debug("Start device scanner, known devices ({})", m_devices.size());

    m_ssdpClient.run();
    m_timer.expires_from_now(s_timeCheckInterval);
    m_timer.async_wait([this] (const asio_error_code& e) {
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

std::shared_ptr<Device> DeviceScanner::getDevice(std::string_view udn) const
{
    auto iter = std::find_if(m_devices.begin(), m_devices.end(), [&udn] (auto& dev) {
        return dev->udn == udn;
    });

    return iter == m_devices.end() ? nullptr : *iter;
}

std::vector<std::shared_ptr<Device>> DeviceScanner::getDevices() const
{
    return m_devices;
}

void DeviceScanner::checkForDeviceTimeouts()
{
    auto now = std::chrono::system_clock::now();

    std::remove_if(m_devices.begin(), m_devices.end(), [this, now] (auto& dev) -> bool {
        if (now < dev->timeoutTime)
        {
            return false;
        }

        log::info("Device timed out removing it from the list: {}", dev->friendlyName);
        DeviceDissapearedEvent(dev);
        return true;
    });
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

    auto iter = std::find_if(m_devices.begin(), m_devices.end(), [&info] (auto& dev) {
        return dev->udn == info.deviceId;
    });

    if (iter != m_devices.end())
    {
        auto device = *iter;

        // device already known, just update the timeout time
        device->timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(info.expirationTime);

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

    auto device = std::make_shared<Device>();
    device->location = info.location;
    device->udn = info.deviceId;
    device->timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(info.expirationTime);

    log::debug("download xml: {}", info.location);
    downloadDeviceXml(info.location, [device, self = std::weak_ptr<DeviceScanner>(m_self)] (const std::string& xml) {
        try
        {
            auto selfPtr = self.lock();
            if (!selfPtr)
            {
                // scanner is destroyed during http get
                return;
            }

            xml::parseDeviceInfo(xml, *device);

            auto iter = std::find_if(selfPtr->m_devices.begin(), selfPtr->m_devices.end(), [device] (auto& dev) {
                return dev->udn == device->udn;
            });

            if (iter == selfPtr->m_devices.end())
            {
                log::info("Device added to the list: {} ({})", device->friendlyName, device->udn);
                selfPtr->m_devices.emplace_back(device);
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
    m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(), [this, &info] (auto& dev) -> bool {
        if (dev->udn == info.deviceId)
        {
            DeviceDissapearedEvent(dev);
            log::info("Device removed from the list: {} ({})", dev->friendlyName, dev->udn);
            return true;
        }

        return false;
    }), m_devices.end());
}

}
