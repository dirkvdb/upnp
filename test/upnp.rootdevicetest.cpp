#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp.client.h"
#include "upnp/upnp.rootdevice.h"
#include "upnp/upnp.asio.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.devicescanner.h"
#include "upnp/upnp.soap.types.h"

#include <future>

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string simpleRootDesc =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "   <device>"
    "       <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>"
    "       <UDN>{}</UDN>"
    "       <friendlyName>NAS</friendlyName>"
    "       <UPC></UPC>"
    "       <presentationURL>{}</presentationURL>"
    "       <serviceList/>"
    "   </device>"
    "</root>";

struct DeviceCallbackMock
{
    MOCK_METHOD1(onActionRequest, std::string(const ActionRequest&));
    MOCK_METHOD1(onSubscriptionRequest, std::string(const SubscriptionRequest&));
};

std::string getLoopbackInterface()
{
    for (auto& intf : getNetworkInterfaces())
    {
        if (intf.address.is_v4() && intf.isLoopback)
        {
            return intf.name;
        }
    }

    throw std::runtime_error("Failed to get loopback interface");
}

class RootDeviceTest : public Test
{
public:
    RootDeviceTest()
    : device(1800s)
    {
        deviceInfo.type = DeviceType(DeviceType::MediaRenderer, 1);
        deviceInfo.udn = "uuid:de5d6118-bfcb-918e-0000-00001eccef34";
        deviceInfo.location = "/rootdesc.xml";
        deviceInfo.friendlyName = "TestDevice";

        device.ControlActionRequested = [&] (auto& arg) { return mock.onActionRequest(arg); };

        client.initialize();
        device.initialize(getLoopbackInterface());

        device.registerDevice(fmt::format(simpleRootDesc, deviceInfo.udn, deviceInfo.location), deviceInfo);

        WaitForDeviceAlive();
    }

    ~RootDeviceTest()
    {
        client.uninitialize();
        device.uninitialize();
    }

    void WaitForDeviceAlive()
    {
        DeviceScanner scanner(client, deviceInfo.type);
        scanner.start();
        scanner.refresh();

        std::promise<void> prom;
        auto fut = prom.get_future();
        scanner.DeviceDiscoveredEvent.connect([&] (auto dev) {
            if (dev->udn == device.getUniqueDeviceName())
            {
                prom.set_value();
            }
        }, this);

        if (fut.wait_for(3s) != std::future_status::ready)
        {
            scanner.refresh();
            EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
        }
    }

    Client client;
    Device deviceInfo;
    DeviceCallbackMock mock;
    RootDevice device;
};

TEST_F(RootDeviceTest, ControlAction)
{
    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });
    action.addArgument("Channel", "Master");
    action.addArgument("DesiredVolume", "49");
    action.addArgument("InstanceID", "0");

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Invoke([&] (const ActionRequest& req) {
        EXPECT_EQ("urn:schemas-upnp-org:service:RenderingControl:1", req.serviceType);
        EXPECT_EQ("SetVolume"s, req.actionName);
        return "Success";
    }));

    std::promise<void> prom;
    auto fut = prom.get_future();
    client.sendAction(action, [&] (Status s, soap::ActionResult actionResult) {
        EXPECT_TRUE(s);
        EXPECT_EQ(200u, s.getStatusCode());
        EXPECT_FALSE(actionResult.fault);
        EXPECT_EQ("Success", actionResult.contents);
        prom.set_value();
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

TEST_F(RootDeviceTest, ControlActionError)
{
    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Throw(std::runtime_error("")));

    std::promise<void> prom;
    auto fut = prom.get_future();
    client.sendAction(action, [&] (Status s, soap::ActionResult actionResult) {
        EXPECT_FALSE(s);
        EXPECT_EQ(500u, s.getStatusCode());
        EXPECT_TRUE(actionResult.fault);
        EXPECT_EQ(403u, actionResult.fault->errorCode);
        EXPECT_EQ("Invalid request"s, actionResult.fault->errorDescription);
        prom.set_value();
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

}
}
