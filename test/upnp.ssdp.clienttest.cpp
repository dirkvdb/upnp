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

    SECTION("TimerTestOnce", "[uv]")
    {
        uv::Timer t(m_loop);

        t.start(50ms, 0ms, [=] () {
            utils::log::info("Timer done");
        });

        m_loop.run(uv::RunMode::Once);
    }

    SECTION("TimerTest", "[uv]")
    {
        uv::Timer t(m_loop);

        int count = 0;

        t.start(50ms, 10ms, [&] () {
            utils::log::info("Timer done");

            if (++count == 3)
            {
                t.stop();
                stopLoopAndCloseRequests(m_loop);
            }
        });

        m_loop.run(uv::RunMode::Default);
    }

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

TEST_CASE("List interfaces", "[uv]")
{
    for (auto& addr : uv::getNetworkInterfaces())
    {
        std::cout << "Name: " << addr.name << std::endl
                  << "Addr: " << addr.ipName() << std::endl
                  << "Internal:" << addr.isInternal << std::endl;
    }
}

}
}
