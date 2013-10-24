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
#include "upnp/upnpclient.h"
#include "upnp/upnptypes.h"

#include "utils/log.h"

#include <chrono>
#include <algorithm>
#include <upnp/upnptools.h>

using namespace utils;
using namespace std::placeholders;
using namespace std::chrono;

namespace upnp
{

static const int timeCheckInterval = 60;

DeviceScanner::DeviceScanner(IClient& client, Device::Type type)
: m_Client(client)
, m_Type(type)
, m_Started(false)
, m_Stop(false)
{
}

DeviceScanner::~DeviceScanner() throw()
{
    stop();
}
    
void DeviceScanner::onDeviceDissapeared(const std::string& deviceId)
{
    std::lock_guard<std::mutex> lock(m_DataMutex);
    auto iter = m_Devices.find(deviceId);
    if (iter != m_Devices.end())
    {
        DeviceDissapearedEvent(iter->second);
        m_Devices.erase(iter);
    }
}

void DeviceScanner::start()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Started)
    {
        return;
    }
    
    log::debug("Start device scanner, known devices (%d)", m_Devices.size());

    m_Client.UPnPDeviceDiscoveredEvent.connect(std::bind(&DeviceScanner::onDeviceDiscovered, this, _1), this);
    m_Client.UPnPDeviceDissapearedEvent.connect(std::bind(&DeviceScanner::onDeviceDissapeared, this, _1), this);
    
    m_Thread = std::async(std::launch::async, std::bind(&DeviceScanner::checkForTimeoutThread, this));
    m_Started = true;
}

void DeviceScanner::stop()
{
    {   
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_Started)
        {
            return;
        }

        m_Client.UPnPDeviceDiscoveredEvent.disconnect(this);
        m_Client.UPnPDeviceDissapearedEvent.disconnect(this);
        
        m_Stop = true;
        m_Condition.notify_all();
    }
    
    m_Thread.wait();
    m_Stop = false;
    m_Started = false;
    
    log::debug("Stop device scanner, known devices (%d)", m_Devices.size());
}

void DeviceScanner::checkForTimeoutThread()
{
    while (!m_Stop)
    {
        std::unique_lock<std::mutex> lock(m_DataMutex);
    
        system_clock::time_point now = system_clock::now();
        auto mapEnd = m_Devices.end();
        for (auto iter = m_Devices.begin(); iter != mapEnd;)
        {
            if (now > iter->second->m_TimeoutTime)
            {
                auto dev = iter->second;
            
                log::info("Device timed out removing it from the list: %s", iter->second->m_FriendlyName);
                iter = m_Devices.erase(iter);
                
                DeviceDissapearedEvent(dev);
            }
            else
            {
                ++iter;
            }
        }
        
        
        if (std::cv_status::no_timeout == m_Condition.wait_for(lock, seconds(timeCheckInterval)))
        {
            return;
        }
    }
}

void DeviceScanner::refresh()
{
    m_Client.searchDevices(m_Type, 5);
}

uint32_t DeviceScanner::getDeviceCount()
{
    std::lock_guard<std::mutex> lock(m_DataMutex);
    return static_cast<uint32_t>(m_Devices.size());
}

std::map<std::string, std::shared_ptr<Device>> DeviceScanner::getDevices()
{
    std::lock_guard<std::mutex> lock(m_DataMutex);
    return m_Devices;
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

bool DeviceScanner::findAndParseService(xml::Document& doc, const ServiceType serviceType, std::shared_ptr<Device>& device)
{
    bool found = false;
    
    xml::NodeList serviceList = getFirstServiceList(doc);
    if (!serviceList)
    {
        return found;
    }
    
    std::string base = device->m_BaseURL.empty() ? device->m_Location : device->m_BaseURL;
    
    unsigned long numServices = serviceList.size();
    for (unsigned long i = 0; i < numServices; ++i)
    {
        xml::Element serviceElem = serviceList.getNode(i);
        
        Service service;
        service.m_Type = serviceTypeUrnStringToService(serviceElem.getChildNodeValue("serviceType"));
        if (service.m_Type == serviceType)
        {
            service.m_Id                    = serviceElem.getChildNodeValue("serviceId");
            std::string relControlURL       = serviceElem.getChildNodeValue("controlURL");
            std::string relEventURL         = serviceElem.getChildNodeValue("eventSubURL");
            std::string scpURL              = serviceElem.getChildNodeValue("SCPDURL");
            
            char url[512];
            int ret = UpnpResolveURL(base.c_str(), relControlURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating controlURL from %s and %s", base, relControlURL);
            }
            else
            {
                service.m_ControlURL = url;
            }
            
            ret = UpnpResolveURL(base.c_str(), relEventURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating eventURL from %s and %s", base, relEventURL);
            }
            else
            {
                service.m_EventSubscriptionURL = url;
            }

            ret = UpnpResolveURL(base.c_str(), scpURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating eventURL from %s and %s", base, scpURL);
            }
            else
            {
                service.m_SCPDUrl = url;
            }
            
            device->m_Services[serviceType] = service;
            
            found = true;
            break;
        }
    }
    
    return found;
}


void DeviceScanner::onDeviceDiscovered(Upnp_Discovery* pDiscovery)
{
    if (m_Type != Device::stringToDeviceType(pDiscovery->DeviceType))
    {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(m_DataMutex);
        
        auto iter = m_Devices.find(pDiscovery->DeviceId);
        if (iter != m_Devices.end())
        {
            // device already known, just update the timeout time
            iter->second->m_TimeoutTime =  system_clock::now() + seconds(pDiscovery->Expires);
            return;
        }
    }
    
    try
    {
        xml::Document doc = m_Client.downloadXmlDocument(pDiscovery->Location);
        auto device = std::make_shared<Device>();
        
        device->m_Location      = pDiscovery->Location;
        device->m_UDN           = doc.getChildNodeValueRecursive("UDN");
        device->m_Type          = Device::stringToDeviceType(doc.getChildNodeValueRecursive("deviceType"));
        device->m_TimeoutTime   = system_clock::now() + seconds(pDiscovery->Expires);
        
        if (device->m_UDN.empty() || device->m_Type != m_Type)
        {
            return;
        }
        
        device->m_FriendlyName   = doc.getChildNodeValueRecursive("friendlyName");
        try { device->m_BaseURL  = doc.getChildNodeValueRecursive("URLBase"); } catch (std::exception&) {}
        try { device->m_RelURL   = doc.getChildNodeValueRecursive("presentationURL"); } catch (std::exception&) {}
        
        char presURL[200];
        int ret = UpnpResolveURL((device->m_BaseURL.empty() ? device->m_BaseURL.c_str() : pDiscovery->Location), device->m_RelURL.empty() ? nullptr : device->m_RelURL.c_str(), presURL);
        if (UPNP_E_SUCCESS == ret)
        {
            device->m_PresURL = presURL;
        }
        
        if (device->m_Type == Device::MediaServer)
        {
            if (findAndParseService(doc, ServiceType::ContentDirectory, device))
            {
                // try to obtain the optional services
                findAndParseService(doc, ServiceType::AVTransport, device);
                findAndParseService(doc, ServiceType::ConnectionManager, device);
                
                log::info("Media server added to the list: %s (%s)", device->m_FriendlyName, device->m_UDN);

                {
                    std::lock_guard<std::mutex> lock(m_DataMutex);
                    m_Devices[device->m_UDN] = device;
                }

                DeviceDiscoveredEvent(device);
            }
        }
        else if (device->m_Type == Device::MediaRenderer)
        {
            if (   findAndParseService(doc, ServiceType::RenderingControl, device)
                && findAndParseService(doc, ServiceType::ConnectionManager, device)
                )
            {
                // try to obtain the optional services
                findAndParseService(doc, ServiceType::AVTransport, device);
                
                log::info("Media renderer added to the list: %s (%s)", device->m_FriendlyName, device->m_UDN);
                
                {
                    std::lock_guard<std::mutex> lock(m_DataMutex);
                    m_Devices[device->m_UDN] = device;
                }
                
                DeviceDiscoveredEvent(device);
            }
        }
    }
    catch (std::exception& e)
    {
        log::error(e.what());
    }
}

}
