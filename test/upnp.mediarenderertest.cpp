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

#include "upnp/upnp.mediarenderer.h"
#include "upnp/upnp.item.h"
#include "upnp/upnp.types.h"
#include "upnp.clientmock.h"
#include "testutils.h"
#include "testxmls.h"

#include <future>
#include <gtest/gtest.h>

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;

namespace
{

void addServiceToDevice(Device& dev, ServiceType type, const std::string& scpUrl, const std::string& controlUrl)
{
    Service svc;
    svc.type = type;
    svc.scpdUrl = scpUrl;
    svc.controlURL = controlUrl;

    dev.services.emplace(type.type, svc);
}

}

class MediaRendererTest : public Test
{
protected:
    MediaRendererTest()
    : m_renderer(m_client)
    , m_device(std::make_shared<Device>())
    {
        std::promise<ErrorCode> promise;
        auto fut = promise.get_future();

        addServiceToDevice(*m_device, { ServiceType::ConnectionManager, 1 }, "CMSCPUrl", "CMCurl");
        addServiceToDevice(*m_device, { ServiceType::RenderingControl, 1 }, "RCCPUrl", "RCCurl");
        addServiceToDevice(*m_device, { ServiceType::AVTransport, 1 }, "AVTCPUrl", "AVTCurl");

        EXPECT_CALL(m_client, getFile("CMSCPUrl", _)).WillOnce(InvokeArgument<1>(Status(), testxmls::connectionManagerServiceDescription));
        EXPECT_CALL(m_client, getFile("RCCPUrl", _)).WillOnce(InvokeArgument<1>(Status(), testxmls::renderingServiceDescription));
        EXPECT_CALL(m_client, getFile("AVTCPUrl", _)).WillOnce(InvokeArgument<1>(Status(), testxmls::avtransportServiceDescription));

        Action getProtoInfo("GetProtocolInfo", "CMCurl", { ServiceType::ConnectionManager, 1 });
        expectAction(getProtoInfo, { { "Source", "http-get:*:*:*" },
                                     { "Sink", "http-get:*:audio/mpeg:*,http-get:*:audio/mp4:*" } });

        m_renderer.setDevice(m_device, [&] (Status status) {
            auto p = std::move(promise);
            p.set_value(status.getErrorCode());
        });

        EXPECT_EQ(ErrorCode::Success, fut.get());
    }

    void expectAction(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        using namespace ContentDirectory;
        EXPECT_CALL(m_client, sendAction(_, _)).WillOnce(Invoke([&, responseVars] (auto& action, auto& cb) {
            EXPECT_EQ(expected.toString(), action.toString());
            cb(Status(), wrapSoap(generateActionResponse(expected.getName(), expected.getServiceType(), responseVars)));
        }));
    }

    ClientMock                  m_client;
    MediaRenderer               m_renderer;
    std::shared_ptr<Device>     m_device;
};

TEST_F(MediaRendererTest, SupportedProtocols)
{
    std::vector<std::string> supportedProtocols
    {
        "http-get:*:audio/mpeg:*",
        "http-get:*:audio/mp4:*"
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
