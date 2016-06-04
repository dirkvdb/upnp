#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp.client.h"
#include "upnp/upnp.rootdevice.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.devicescanner.h"

#include <future>

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

#ifdef __linux__
static const std::string s_interface = "lo";
#elif defined __APPLE__
static const std::string s_interface = "lo0";
#else
static const std::string s_interface = "localhost";
#endif

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

TEST(RootDeviceTest, ControlAction)
{
    Client client;
    RootDevice device(1800s);

    Device deviceInfo;
    deviceInfo.type = DeviceType(DeviceType::MediaRenderer, 1);
    deviceInfo.udn = "uuid:de5d6118-bfcb-918e-0000-00001eccef34";

    DeviceScanner scanner(client, deviceInfo.type);
    DeviceCallbackMock mock;

    client.initialize();
    device.initialize(s_interface);

    deviceInfo.location = "/rootdesc.xml";
    device.registerDevice(fmt::format(simpleRootDesc, deviceInfo.udn, deviceInfo.location), deviceInfo);

    device.ControlActionRequested = [&] (auto& arg) { return mock.onActionRequest(arg); };

    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });
    action.addArgument("Channel", "Master");
    action.addArgument("DesiredVolume", "49");
    action.addArgument("InstanceID", "0");

    std::promise<void> prom;
    auto fut = prom.get_future();

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Invoke([&] (const ActionRequest& req) {
        EXPECT_EQ("urn:schemas-upnp-org:service:RenderingControl:1", req.serviceType);
        EXPECT_EQ("SetVolume"s, req.actionName);

        return "Success";
    }));

    scanner.DeviceDiscoveredEvent.connect([&] (auto dev) {
        if (dev->udn == device.getUniqueDeviceName())
        {
            client.sendAction(action, [&] (Status s, std::string actionResult) {
                EXPECT_TRUE(s);
                EXPECT_EQ("Success", actionResult);
                prom.set_value();
            });
        }
    }, &client);

    scanner.start();
    scanner.refresh();

    EXPECT_EQ(std::future_status::ready, fut.wait_for(5s));
    client.uninitialize();
    device.uninitialize();
}

}
}
