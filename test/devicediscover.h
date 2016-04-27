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

#include <condition_variable>
#include "upnp/upnpdevice.h"
#include "upnp/upnp.devicescanner.h"

namespace upnp
{
namespace test
{

class DeviceDiscover
{
public:
    DeviceDiscover(IClient2& client, DeviceType type, const std::string& name)
    : m_scanner(client, type)
    , m_deviceName(name)
    {
        m_scanner.DeviceDiscoveredEvent.connect(std::bind(&DeviceDiscover::deviceDiscovered, this, std::placeholders::_1), this);
        m_scanner.start();
    }

    void refresh()
    {
        m_scanner.refresh();
    }

    void waitForDevice(std::chrono::milliseconds timeout)
    {
        auto fut = m_promise.get_future();
        fut.wait_for(timeout);
        fut.get();
        m_scanner.stop();
    }

    void deviceDiscovered(std::shared_ptr<upnp::Device> dev)
    {
        if (dev->m_friendlyName == m_deviceName)
        {
            m_device = dev;
            m_promise.set_value();
        }
    }

    std::shared_ptr<Device> getDevice()
    {
        return m_device;
    }

private:
    upnp::DeviceScanner             m_scanner;
    std::string                     m_deviceName;
    std::shared_ptr<Device>         m_device;
    std::promise<void>              m_promise;
};

}
}
