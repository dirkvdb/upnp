
#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.ssdp.client.h"

namespace upnp
{
namespace test
{

using namespace std::chrono_literals;

TEST(SSDPClientTest, DISABLED_Client)
{
    asio::io_service io;
    ssdp::Client client(io);

    utils::log::info("SsdpServer");

    client.setDeviceNotificationCallback([&] (auto&& msg) {
        utils::log::warn("@@@ DiscoverCallback ID {} TYPE {} EXP {}", msg.deviceId, msg.deviceType, msg.expirationTime);
        io.stop();
    });

    client.run();
    client.search();

    auto t = std::thread([&] () {
        io.run();
    });

    t.join();
}

}
}
