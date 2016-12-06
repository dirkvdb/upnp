#include <gtest/gtest.h>
#include <future>

#include "upnp/upnp.asio.h"
#include "upnp/upnp.lastchangevariable.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::chrono_literals;

TEST(LastChangeVariableTest, Generate)
{
    asio::io_service io;
    ServiceType svcType;
    svcType.type = ServiceType::RenderingControl;
    svcType.version = 1;

    std::promise<std::string> prom;

    LastChangeVariable var(io, svcType, 5s);
    var.LastChangeEvent = [&] (const std::string& s) {
        prom.set_value(s);
    };

    var.addChangedVariable(1, ServiceVariable("VarName", "Value"));

    std::string expected =
        "<?xml version=\"1.0\"?>"
        "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\" e:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "<e:property>"
        "<LastChange>&amp;lt;Event xmlns:e=&amp;quot;urn:schemas-upnp-org:metadata-1-0/RCS/&amp;quot;&amp;gt;&amp;lt;InstanceID val=&amp;quot;1&amp;quot;&amp;gt;&amp;lt;VarName val=&amp;quot;Value&amp;quot;/&amp;gt;&amp;lt;/InstanceID&amp;gt;&amp;lt;/Event&amp;gt;</LastChange>"
        "</e:property>"
        "</e:propertyset>";

    auto fut = prom.get_future();
    ASSERT_EQ(std::future_status::ready, fut.wait_for(3s));
    EXPECT_EQ(expected, fut.get());
}

}
}
