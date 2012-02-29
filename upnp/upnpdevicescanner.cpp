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

#include "upnpdevicescanner.h"
#include "upnpcontrolpoint.h"

#include "utils/log.h"

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

DeviceScanner::DeviceScanner(ControlPoint& controlPoint, DeviceType type)
: m_ControlPoint(controlPoint)
, m_Type(type)
{
}
    
void DeviceScanner::onUPnPDeviceDiscovered(const Device& device)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::map<std::string, Device>::iterator iter = m_Devices.find(device.m_UDN);
    if (iter == m_Devices.end())
    {
        log::info("Device discovered:", device.m_FriendlyName, device.m_UDN);
        m_Devices[device.m_UDN] = device;
        DeviceDiscoveredEvent(device);
    }
}

void DeviceScanner::onUPnPDeviceDissapeared(const Device& device)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::map<std::string, Device>::iterator iter = m_Devices.find(device.m_UDN);
    if (iter != m_Devices.end())
    {
        log::info("Device dissapeared:", device.m_UDN);
        m_Devices.erase(iter);
        DeviceDissapearedEvent(device);
    }
}

void DeviceScanner::start()
{
    if (m_Type == Servers)
    {
        m_ControlPoint.DeviceDiscoveredEvent.connect(std::bind(&DeviceScanner::onUPnPDeviceDiscovered, this, _1), this);
        m_ControlPoint.DeviceDissapearedEvent.connect(std::bind(&DeviceScanner::onUPnPDeviceDissapeared, this, _1), this);
    }
}
    
void DeviceScanner::stop()
{
    if (m_Type == Servers)
    {
        m_ControlPoint.DeviceDiscoveredEvent.disconnect(this);
        m_ControlPoint.DeviceDissapearedEvent.disconnect(this);
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
