#include <gtest/gtest.h>
#include <future>

#include "utils/log.h"
#include "upnp/upnp.devicescanner.h"
#include "upnp/upnp.client.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::chrono_literals;

TEST(DeviceDiscoverTest, DiscoverClient)
{
    Client2 client;
    DeviceScanner scanner(client, { DeviceType::MediaServer, DeviceType::MediaRenderer });

    std::promise<void> prom;
    auto fut = prom.get_future();

    scanner.DeviceDiscoveredEvent.connect([&] (std::shared_ptr<Device> dev) {
        log::info("Discovered: {}", dev->m_udn);
        scanner.stop();
        prom.set_value();
    }, &client);

    scanner.start();
    scanner.refresh();

    client.initialize("lo0", 0);
    fut.wait();
    client.uninitialize();
}

}
}
