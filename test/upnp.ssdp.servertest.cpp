
#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.ssdp.client.h"
#include "upnp/upnp.ssdp.server.h"

#include "upnp/upnp.device.h"

namespace upnp
{
namespace test
{

using namespace std::chrono_literals;
using namespace testing;

class SsdpServerTest : public Test
{
public:
    SsdpServerTest()
    : server(loop)
    , client(loop)
    {
        Device dev;
        dev.location = "http://localhost/";
        dev.type = DeviceType(DeviceType::MediaRenderer, 1);

        Service svc;

        svc.type = ServiceType(ServiceType::ConnectionManager, 1);
        dev.services.emplace(svc.type.type, svc);

        svc.type = ServiceType(ServiceType::RenderingControl, 1);
        dev.services.emplace(svc.type.type, svc);

        server.run(dev);
    }

    uv::Loop loop;
    ssdp::Server server;
    ssdp::Client client;
};

TEST_F(SsdpServerTest, DISABLED_Server)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        utils::log::warn("@@@ DiscoverCallback ID {} TYPE {} EXP {}", msg.deviceId, msg.deviceType, msg.expirationTime);
        //m_client.stop();
    });

    client.run();
    client.search();

    loop.run(uv::RunMode::Default);
}

}
}
