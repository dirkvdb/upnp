#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.devicescanner.h"
#include "upnp/upnp.uv.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::chrono_literals;

TEST(DeviceDiscoverTest, DiscoverClient)
{
    uv::Loop loop;
    DeviceScanner scanner(loop, { DeviceType::MediaServer, DeviceType::MediaRenderer });

    scanner.DeviceDiscoveredEvent.connect([&] (std::shared_ptr<Device> dev) {
        log::info("Discovered: {}", dev->m_udn);
        scanner.stop();
    }, &loop);

    scanner.start();
    scanner.refresh();

    loop.run(uv::RunMode::Default);
}

}
}
