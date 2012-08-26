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
    : m_Renderer(g_Env->getClient())
    {
    }

protected:
    virtual void SetUp()
    {
        m_Device = g_Env->getRenderer();
        ASSERT_TRUE(m_Device.get() != nullptr);

        m_Renderer.setDevice(m_Device);
    }

    virtual void TearDown()
    {
    }

    MediaRenderer               m_Renderer;    
    std::shared_ptr<Device>     m_Device;
};

TEST_F(MediaRendererTest, DiscoveredServices)
{
    EXPECT_FALSE(m_Device->m_Services.end() == m_Device->m_Services.find(ServiceType::RenderingControl));
    EXPECT_FALSE(m_Device->m_Services.end() == m_Device->m_Services.find(ServiceType::ConnectionManager));
    EXPECT_FALSE(m_Device->m_Services.end() == m_Device->m_Services.find(ServiceType::AVTransport));
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

        auto item = std::make_shared<Item>();
        item->addResource(res);

        Resource suggestedResource;
        EXPECT_TRUE(m_Renderer.supportsPlayback(item, suggestedResource)) << "Protocol not supported: " << item->getResources()[0].getProtocolInfo().toString();
    }
}

}
}
