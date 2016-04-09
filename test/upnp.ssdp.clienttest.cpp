#include <catch.hpp>

#include "utils/log.h"
#include "upnp/upnp.ssdp.client.h"

namespace upnp
{
namespace test
{

using namespace std::chrono_literals;

TEST_CASE("SSDP Client", "[SSDP]")
{
    uv::Loop        loop;
    ssdp::Client    client(loop);

    SECTION("Ssdp client", "[SSDP]")
    {
        utils::log::info("SsdpServer");

        client.setDeviceNotificationCallback([&] (auto&& msg) {
            utils::log::warn("@@@ DiscoverCallback ID {} TYPE {} EXP {}", msg.deviceId, msg.deviceType, msg.expirationTime);
            loop.stop();
        });

        client.run();
        client.search();

        auto t = std::thread([&] () {
            loop.run(uv::RunMode::Default);
        });

        t.join();
    }
}

}
}
