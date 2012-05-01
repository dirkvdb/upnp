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

#ifndef TEST_ENVIRONMENT_H
#define TEST_ENVIRONMENT_H

#include <gtest/gtest.h>

#include "upnp/upnpclient.h"
#include "devicediscover.h"

#define SERVER_DEVICE "Plug"
#define RENDERER_DEVICE "Pioneer"

namespace upnp
{
namespace test
{

class TestEnvironment : public Environment
{
public:
    TestEnvironment()
    : m_ServerDiscoverer(m_Client, Device::MediaServer, SERVER_DEVICE)
    , m_RendererDiscoverer(m_Client, Device::MediaRenderer, RENDERER_DEVICE)
    {

    }

    virtual ~TestEnvironment() {}
  
    virtual void SetUp()
    {
        m_Client.initialize();
        m_ServerDiscoverer.refresh();
        m_RendererDiscoverer.refresh();
        m_ServerDiscoverer.waitForDevice(10000);
        m_RendererDiscoverer.waitForDevice(10000);
        m_ServerDevice = m_ServerDiscoverer.getDevice();
        m_RendererDevice = m_RendererDiscoverer.getDevice();
        ASSERT_TRUE(m_ServerDevice.get() != nullptr) << "Failed to obtain UPnP server " << SERVER_DEVICE;
        ASSERT_TRUE(m_RendererDevice.get() != nullptr) << "Failed to obtain UPnP renderer " << RENDERER_DEVICE;
    }

    virtual void TearDown()
    {
        m_Client.destroy();
    }

    Client& getClient()
    {
        return m_Client;
    }

    std::shared_ptr<Device> getServer()
    {
        return m_ServerDevice;
    }

    std::shared_ptr<Device> getRenderer()
    {
        return m_RendererDevice;
    }

private:
    Client                      m_Client;
    DeviceDiscover              m_ServerDiscoverer;
    DeviceDiscover              m_RendererDiscoverer;
    std::shared_ptr<Device>     m_ServerDevice;
    std::shared_ptr<Device>     m_RendererDevice;
};

}
}

#endif
