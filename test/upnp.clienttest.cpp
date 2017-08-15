#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp.client.h"

#include <future>

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST(UPnPClientTest, DISABLED_Client)
{
    Client client;

    auto url = "http://192.168.1.184:8080/RenderingControl/evt"s;
    std::promise<void> prom;
    auto fut = prom.get_future();

    client.subscribeToService(url, 1000s, [&client, &url, &prom] (int32_t status, std::string sid, std::chrono::seconds timeout) -> std::function<void(SubscriptionEvent)> {
        EXPECT_EQ(200, status);
        EXPECT_EQ(300, timeout.count());
        log::info("SubId: {}", sid);

        return [=, &client, &prom] (const SubscriptionEvent& ev) {
            EXPECT_EQ(sid, ev.sid);
            EXPECT_FALSE(ev.data.empty());

            client.unsubscribeFromService(url, sid, [&prom] (int32_t status) {
                EXPECT_EQ(200, status);
                prom.set_value();
            });
        };
    });

    client.initialize("en0", 0);
    fut.wait();
    client.uninitialize();
}

}
}
