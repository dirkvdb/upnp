//
//  upnpdevicescanner.cpp
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 03/06/11.
//  Copyright 2011 None. All rights reserved.
//

#include "upnpdevicescanner.h"
#include "upnpcontrolpoint.h"

#include <Utils/log.h>

namespace UPnP
{

DeviceScanner::DeviceScanner(ControlPoint& controlPoint, DeviceType type)
: m_ControlPoint(controlPoint)
, m_Type(type)
, m_pListener(nullptr)
{
}
    
void DeviceScanner::onUPnPDeviceEvent(const Device& device, DeviceEvent event)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::map<std::string, Device>::iterator iter = m_Devices.find(device.m_UDN);
    if (iter == m_Devices.end())
    {
        if (event == deviceDiscovered)
        {
            Log::info("Device discovered:", device.m_FriendlyName, device.m_UDN);
            m_Devices[device.m_UDN] = device;
            if (m_pListener) m_pListener->onUPnPDeviceEvent(device, event);
        }
    }
    else if (event == deviceDissapeared)
    {
        Log::info("Device dissapeared:", device.m_UDN);
        m_Devices.erase(iter);
        if (m_pListener) m_pListener->onUPnPDeviceEvent(device, event);
    }
}

void DeviceScanner::setListener(IDeviceSubscriber& listener)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    m_pListener = &listener;

    // make sure the new listeners gets all the devices that are already known
    for (const auto devicePair : m_Devices)
    {
        m_pListener->onUPnPDeviceEvent(devicePair.second, IDeviceSubscriber::deviceDiscovered);
    }    
}
    
void DeviceScanner::start()
{
    if (m_Type == Servers)
    {
        m_ControlPoint.getServersASync(*this);
    }
}
    
void DeviceScanner::stop()
{
    if (m_Type == Servers)
    {
        m_ControlPoint.stopReceivingServers(*this);
    }
}

void DeviceScanner::refresh()
{
    m_ControlPoint.manualDiscovery();
}

uint32_t DeviceScanner::getDeviceCount()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Devices.size();
}

std::map<std::string, Device> DeviceScanner::getDevices()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Devices;
}

}
