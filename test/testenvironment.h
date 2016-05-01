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

#include "gtest/gtest.h"

#include "upnp/upnp.clientinterface.h"
#include "upnp/upnp.factory.h"
#include "devicediscover.h"

#define SERVER_DEVICE "Plug"
#define RENDERER_DEVICE "Pioneer"

namespace upnp
{
namespace test
{

using namespace std::chrono_literals;

class TestEnvironment : public Environment
{
public:
    TestEnvironment()
    : m_client(factory::createClient())
    , m_serverDiscoverer(*m_client, DeviceType::MediaServer, SERVER_DEVICE)
    , m_rendererDiscoverer(*m_client, DeviceType::MediaRenderer, RENDERER_DEVICE)
    {

    }

    virtual void SetUp()
    {
        m_client->initialize();
        m_serverDiscoverer.refresh();
        m_rendererDiscoverer.refresh();
        m_serverDiscoverer.waitForDevice(10000ms);
        m_rendererDiscoverer.waitForDevice(10000ms);
        m_serverDevice = m_serverDiscoverer.getDevice();
        m_rendererDevice = m_rendererDiscoverer.getDevice();
        ASSERT_TRUE(m_serverDevice) << "Failed to obtain UPnP server " << SERVER_DEVICE;
        ASSERT_TRUE(m_rendererDevice) << "Failed to obtain UPnP renderer " << RENDERER_DEVICE;
    }

    virtual void TearDown()
    {
        m_client->uninitialize();
    }

    IClient2& getClient()
    {
        return *m_client;
    }

    std::shared_ptr<Device> getServer()
    {
        return m_serverDevice;
    }

    std::shared_ptr<Device> getRenderer()
    {
        return m_rendererDevice;
    }

private:
    std::unique_ptr<IClient2>   m_client;
    DeviceDiscover              m_serverDiscoverer;
    DeviceDiscover              m_rendererDiscoverer;
    std::shared_ptr<Device>     m_serverDevice;
    std::shared_ptr<Device>     m_rendererDevice;
};

}
}
