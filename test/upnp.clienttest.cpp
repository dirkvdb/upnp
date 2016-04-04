#include <catch.hpp>

#include "utils/log.h"
#include "upnp/upnp.client.h"
#include "upnp/upnp.uv.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("UPnP Client", "[.][upnp]")
{
    uv::Loop loop;
    Client2 client(loop);
    bool gotCallback = false;
    
    auto url = "http://192.168.1.184:8080/RenderingControl/evt"s;
    
    client.initialize("en0", 0);
    client.subscribeToService(url, 1000s, [&client, &url] (int32_t status, std::string sid, std::chrono::seconds timeout) -> std::function<void(SubscriptionEvent)> {
        CHECK(status == 200);
        CHECK(timeout.count() == 300);
        log::info("SubId: {}", sid);
        
        return [=, &client] (const SubscriptionEvent& ev) {
            CHECK(ev.sid == sid);
            CHECK_FALSE(ev.data.empty());
            
            client.unsubscribeFromService(url, sid, [&client] (int32_t status) {
                CHECK(status == 200);
                client.uninitialize();
        });
        };
    });

    loop.run(uv::RunMode::Default);
    //CHECK(gotCallback);
}

}
}
