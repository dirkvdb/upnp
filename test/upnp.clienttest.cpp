#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.client.h"
#include "upnp/upnp.uv.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST(UPnPClientTest, DISABLED_CLIENT)
{
    uv::Loop loop;
    Client2 client(loop);

    auto url = "http://192.168.1.184:8080/RenderingControl/evt"s;

    client.initialize("en0", 0);
    client.subscribeToService(url, 1000s, [&client, &url] (int32_t status, std::string sid, std::chrono::seconds timeout) -> std::function<void(SubscriptionEvent)> {
        EXPECT_EQ(200, status);
        EXPECT_EQ(300, timeout.count());
        log::info("SubId: {}", sid);

        return [=, &client] (const SubscriptionEvent& ev) {
            EXPECT_EQ(sid, ev.sid);
            EXPECT_FALSE(ev.data.empty());

            client.unsubscribeFromService(url, sid, [&client] (int32_t status) {
                EXPECT_EQ(200, status);
                client.uninitialize();
            });
        };
    });

    loop.run(uv::RunMode::Default);
}

}
}
