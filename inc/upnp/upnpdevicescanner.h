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

#ifndef UPNP_DEVICE_SCANNER
#define UPNP_DEVICE_SCANNER

#include <map>
#include <string>
#include <mutex>

#include "upnpdevice.h"
#include "utils/signal.h"

namespace upnp
{

class ControlPoint;

class DeviceScanner
{
public:
    enum DeviceType
    {
        Servers
    };
    
    DeviceScanner(ControlPoint& controlPoint, DeviceType type);
    
    void start();
    void stop();
    void refresh();
    
    uint32_t getDeviceCount();
    std::map<std::string, Device> getDevices();
    
    void onUPnPDeviceDiscovered(const Device& device);
    void onUPnPDeviceDissapeared(const Device& device);

    utils::Signal<void(const Device&)> DeviceDiscoveredEvent;
    utils::Signal<void(const Device&)> DeviceDissapearedEvent;
    
private:
    ControlPoint&                   m_ControlPoint;
    DeviceType                      m_Type;
    std::map<std::string, Device>   m_Devices;
    std::mutex                      m_Mutex;
};

}

#endif