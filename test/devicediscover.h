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

#ifndef TEST_DEVICE_DISCOVER_H
#define TEST_DEVICE_DISCOVER_H

#include <condition_variable>

#include "upnp/upnpdevicescanner.h"

using std::placeholders::_1;

namespace upnp
{
namespace test
{

class DeviceDiscover
{
public:
    DeviceDiscover(Client& client, Device::Type type, const std::string& name)
    : m_Scanner(client, type)
    , m_DeviceName(name)
    {
        m_Scanner.DeviceDiscoveredEvent.connect(std::bind(&DeviceDiscover::deviceDiscovered, this, _1), this);
        m_Scanner.start();
    }

    void refresh()
    {
        m_Scanner.refresh();
    }

    void waitForDevice(uint32_t timeout)
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        if (m_Device)
        {
            return;
        }
        
        m_Condition.wait_for(lock, std::chrono::milliseconds(timeout));
        m_Scanner.stop();
    }

    void deviceDiscovered(std::shared_ptr<upnp::Device> dev)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (dev->m_FriendlyName == m_DeviceName)
        {
            m_Device = dev;
            m_Condition.notify_all();
        }
    }

    std::shared_ptr<Device> getDevice()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Device;
    }

private:
    upnp::DeviceScanner             m_Scanner;
    std::string                     m_DeviceName;
    std::shared_ptr<Device>         m_Device;

    std::condition_variable         m_Condition;
    std::mutex                      m_Mutex;
};

}
}

#endif
