#include <gtest/gtest.h>
#include <future>

#include "utils/log.h"
#include "upnp/upnp.devicescanner.h"
#include "upnp.client.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::chrono_literals;

TEST(DeviceScannerTest, DiscoverClient)
{
    Client2 client;

    DeviceScanner scanner(client, { DeviceType::MediaServer, DeviceType::MediaRenderer });
    client.initialize();

    std::promise<void> prom;
    auto fut = prom.get_future();
    bool discovered = false;

    scanner.DeviceDiscoveredEvent.connect([&] (std::shared_ptr<Device> dev) {
        if (!discovered)
        {
            discovered = true;
            log::info("Discovered: {}", dev->m_udn);
            scanner.stop();
            prom.set_value();
        }
    }, &client);

    uv::asyncSend(client.loop(), [&] () {
        scanner.start();
        scanner.refresh();
    });

    fut.wait();
    
    scanner.stop();
    client.uninitialize();
}

}
}
