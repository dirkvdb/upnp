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

#include "upnp/upnpdevicescanner.h"
#include "upnp/upnpclientinterface.h"
#include "upnp/upnptypes.h"

#include "utils/log.h"

#include <chrono>
#include <algorithm>
#include <upnptools.h>


namespace upnp
{

using namespace utils;
using namespace std::placeholders;
using namespace std::chrono_literals;

static const auto g_timeCheckInterval = 60s;

DeviceScanner::DeviceScanner(uv::Loop& loop, DeviceType type)
: DeviceScanner(loop, std::set<DeviceType>{ type })
{
}

DeviceScanner::DeviceScanner(uv::Loop& loop, std::set<DeviceType> types)
: m_httpClient(loop)
, m_ssdpClient(loop)
, m_timer(loop)
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

void DeviceScanner::onDeviceDissapeared(const ssdp::DeviceNotificationInfo& info)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_devices.find(info.deviceId);
    if (iter != m_devices.end())
    {
        DeviceDissapearedEvent(iter->second);
        m_devices.erase(iter);
    }
}

void DeviceScanner::start()
{
    log::debug("Start device scanner, known devices ({})", m_devices.size());

    m_ssdpClient.run();
    m_timer.start(g_timeCheckInterval, g_timeCheckInterval, [=] () {
        checkForDeviceTimeouts();
    });
}

void DeviceScanner::stop()
{
    m_timer.stop();
    m_ssdpClient.stop();
    log::debug("Stop device scanner, known devices ({})", m_devices.size());
}

void DeviceScanner::checkForDeviceTimeouts()
{
    auto now = std::chrono::system_clock::now();
    std::unique_lock<std::mutex> lock(m_dataMutex);
    auto mapEnd = m_devices.end();
    for (auto iter = m_devices.begin(); iter != mapEnd;)
    {
        if (now > iter->second->m_timeoutTime)
        {
            auto dev = iter->second;
            log::info("Device timed out removing it from the list: {}", iter->second->m_friendlyName);
            iter = m_devices.erase(iter);
            DeviceDissapearedEvent(dev);
        }
        else
        {
            ++iter;
        }
    }
}

void DeviceScanner::refresh()
{
    if (m_types.size() == 1)
    {
        m_ssdpClient.search(Device::deviceTypeToString(*m_types.begin()));
    }
    else
    {
        m_ssdpClient.search();
    }
}

uint32_t DeviceScanner::getDeviceCount() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return static_cast<uint32_t>(m_devices.size());
}

std::shared_ptr<Device> DeviceScanner::getDevice(const std::string& udn) const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_devices.at(udn);
}

std::map<std::string, std::shared_ptr<Device>> DeviceScanner::getDevices() const
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_devices;
}

void DeviceScanner::downloadDeviceXml(const std::string& url, std::function<void(std::string)> cb)
{
    m_httpClient.get(url, [=] (int32_t status, std::string xml) {
        if (status < 0)
        {
            log::error("Failed to download device info {} ({})", url, http::Client::errorToString(status));
            return;
        }

        cb(std::move(xml));
    });
}

void DeviceScanner::parseDeviceInfo(const std::string& xml, const std::shared_ptr<Device>& device)
{
    xml::Document doc(xml);

    device->m_udn           = doc.getChildNodeValueRecursive("UDN");
    device->m_type          = Device::stringToDeviceType(doc.getChildNodeValueRecursive("deviceType"));

    if (device->m_udn.empty())
    {
        return;
    }

    device->m_friendlyName   = doc.getChildNodeValueRecursive("friendlyName");
    try { device->m_baseURL  = doc.getChildNodeValueRecursive("URLBase"); } catch (std::exception&) {}
    try { device->m_relURL   = doc.getChildNodeValueRecursive("presentationURL"); } catch (std::exception&) {}

    char presURL[200];
    int ret = UpnpResolveURL((device->m_baseURL.empty() ? device->m_baseURL.c_str() : device->m_location.c_str()), device->m_relURL.empty() ? nullptr : device->m_relURL.c_str(), presURL);
    if (UPNP_E_SUCCESS == ret)
    {
        device->m_presURL = presURL;
    }

    if (device->m_type == DeviceType::MediaServer)
    {
        if (findAndParseService(doc, ServiceType::ContentDirectory, device))
        {
            // try to obtain the optional services
            findAndParseService(doc, ServiceType::AVTransport, device);
            findAndParseService(doc, ServiceType::ConnectionManager, device);
        }
    }
    else if (device->m_type == DeviceType::MediaRenderer)
    {
        if (findAndParseService(doc, ServiceType::RenderingControl, device) &&
            findAndParseService(doc, ServiceType::ConnectionManager, device))
        {
            // try to obtain the optional services
            findAndParseService(doc, ServiceType::AVTransport, device);
        }
    }
}

xml::NodeList DeviceScanner::getFirstServiceList(xml::Document& doc)
{
    xml::NodeList serviceList;

    xml::NodeList servlistNodelist = doc.getElementsByTagName("serviceList");
    if (servlistNodelist.size() > 0)
    {
        xml::Element servlistElem = servlistNodelist.getNode(0);
        return servlistElem.getElementsByTagName("service");
    }

    return serviceList;
}

bool DeviceScanner::findAndParseService(xml::Document& doc, const ServiceType serviceType, const std::shared_ptr<Device>& device)
{
    bool found = false;

    xml::NodeList serviceList = getFirstServiceList(doc);
    if (!serviceList)
    {
        return found;
    }

    std::string base = device->m_baseURL.empty() ? device->m_location : device->m_baseURL;

    unsigned long numServices = serviceList.size();
    for (unsigned long i = 0; i < numServices; ++i)
    {
        xml::Element serviceElem = serviceList.getNode(i);

        Service service;
        service.m_type = serviceTypeUrnStringToService(serviceElem.getChildNodeValue("serviceType"));
        if (service.m_type == serviceType)
        {
            service.m_id                    = serviceElem.getChildNodeValue("serviceId");
            std::string relControlURL       = serviceElem.getChildNodeValue("controlURL");
            std::string relEventURL         = serviceElem.getChildNodeValue("eventSubURL");
            std::string scpURL              = serviceElem.getChildNodeValue("SCPDURL");

            char url[512];
            int ret = UpnpResolveURL(base.c_str(), relControlURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating controlURL from {} and {}", base, relControlURL);
            }
            else
            {
                service.m_controlURL = url;
            }

            ret = UpnpResolveURL(base.c_str(), relEventURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating eventURL from {} and {}", base, relEventURL);
            }
            else
            {
                service.m_eventSubscriptionURL = url;
            }

            ret = UpnpResolveURL(base.c_str(), scpURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating eventURL from {} and {}", base, scpURL);
            }
            else
            {
                service.m_scpdUrl = url;
            }

            device->m_services[serviceType] = service;

            found = true;
            break;
        }
    }

    return found;
}


void DeviceScanner::onDeviceDiscovered(const ssdp::DeviceNotificationInfo& info)
{
    auto deviceType = Device::stringToDeviceType(info.deviceType);
    if (m_types.find(deviceType) == m_types.end())
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        auto iter = m_devices.find(info.deviceId);
        if (iter != m_devices.end())
        {
            auto device = iter->second;

            // device already known, just update the timeout time
            device->m_timeoutTime =  std::chrono::system_clock::now() + std::chrono::seconds(info.expirationTime);

            // check if the location is still the same (perhaps a new ip or port)
            if (device->m_location != std::string(info.location))
            {
                // update the device, ip or port has changed
                log::debug("Update device, location has changed: {} -> {}", device->m_location, std::string(info.location));
                downloadDeviceXml(info.location, [this, device, exp = info.expirationTime] (const std::string& xml) {
                    try
                    {
                        device->m_timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(exp);
                        parseDeviceInfo(xml, device);
                    }
                    catch (std::exception& e)
                    {
                        log::error(e.what());
                    }
                });
            }

            return;
        }
    }

    downloadDeviceXml(info.location, [this, loc = info.location, exp = info.expirationTime] (const std::string& xml) {
        try
        {
            auto device = std::make_shared<Device>();
            device->m_location = loc;
            device->m_timeoutTime = std::chrono::system_clock::now() + std::chrono::seconds(exp);
            parseDeviceInfo(xml, device);

            std::lock_guard<std::mutex> lock(m_dataMutex);
            auto iter = m_devices.find(device->m_udn);
            if (iter == m_devices.end())
            {
                log::info("Device added to the list: {} ({})", device->m_friendlyName, device->m_udn);
                m_devices.emplace(device->m_udn, device);

                m_dataMutex.unlock();
                DeviceDiscoveredEvent(device);
            }
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    });
}

}
