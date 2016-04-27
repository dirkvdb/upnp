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

#include "utils/signal.h"

#include <gtest/gtest.h>
#include <iostream>


using namespace utils;
using namespace testing;

#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpmediarenderer.h"
#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"

#include "testenvironment.h"

extern upnp::test::TestEnvironment* g_Env;

namespace upnp
{
namespace test
{

class MediaRendererTest : public Test
{
public:
    MediaRendererTest()
    : m_renderer(g_Env->getClient())
    {
    }

protected:
    void SetUp() override
    {
        m_device = g_Env->getRenderer();
        ASSERT_TRUE(m_device);

        m_renderer.setDevice(m_device);
    }

    MediaRenderer               m_renderer;
    std::shared_ptr<Device>     m_device;
};

TEST_F(MediaRendererTest, DiscoveredServices)
{
    EXPECT_FALSE(m_device->m_services.end() == m_device->m_services.find(ServiceType::RenderingControl));
    EXPECT_FALSE(m_device->m_services.end() == m_device->m_services.find(ServiceType::ConnectionManager));
    EXPECT_FALSE(m_device->m_services.end() == m_device->m_services.find(ServiceType::AVTransport));
}

TEST_F(MediaRendererTest, SupportedProtocols)
{
    std::vector<std::string> supportedProtocols
    {
        "http-get:*:audio/L16;rate=44100;channels=1:*",
        "http-get:*:audio/L16;rate=44100;channels=2:*",
        "http-get:*:audio/L16;rate=48000;channels=1:*",
        "http-get:*:audio/L16;rate=48000;channels=2:*",
        "http-get:*:audio/mpeg:*",
        "http-get:*:audio/x-ms-wma:*",
        "http-get:*:audio/mp4:*",
        "http-get:*:audio/3gpp:*",
        "http-get:*:audio/vnd.dlna.adts:*",
        "http-get:*:audio/wav:*",
        "http-get:*:audio/x-wav:*",
        "http-get:*:audio/flac:*",
        "http-get:*:audio/x-flac:*",
        "http-get:*:image/jpeg:*",
        "http-wavetunes:*:audio/x-ms-wma:*"
    };

    for (auto& protocol : supportedProtocols)
    {
        Resource res;
        res.setProtocolInfo(upnp::ProtocolInfo(protocol));

        Item item;
        item.addResource(res);

        Resource suggestedResource;
        EXPECT_TRUE(m_renderer.supportsPlayback(item, suggestedResource)) << "Protocol not supported: " << item.getResources()[0].getProtocolInfo().toString();
    }
}

}
}
