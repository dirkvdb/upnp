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
    uv::Loop        m_loop;
    ssdp::Client    m_client(m_loop);

    SECTION("Ssdp client", "[SSDP]")
    {
        utils::log::info("SsdpServer");

        m_client.setDeviceNotificationCallback([&] (auto&& msg) {
            utils::log::warn("@@@ DiscoverCallback ID {} TYPE {} EXP {}", msg.deviceId, msg.deviceType, msg.expirationTime);
            m_loop.stop();
        });

        m_client.run();
        m_client.search();

        auto t = std::thread([&] () {
            m_loop.run(uv::RunMode::Default);
        });

        t.join();
    }
}

}
}
