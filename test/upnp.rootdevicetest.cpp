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

static const std::string s_response =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: 7\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n"
    "Success";
    
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
    Client2 client;
    RootDevice2 device(1800s);
    
    Device deviceInfo;
    deviceInfo.type = DeviceType(DeviceType::MediaRenderer, 1);
    deviceInfo.udn = "uuid:de5d6118-bfcb-918e-0000-00001eccef34";
    
    DeviceScanner scanner(client, deviceInfo.type);
    DeviceCallbackMock mock;
    
    client.initialize();
    device.initialize("lo0");
    
    deviceInfo.location = "/rootdesc.xml";
    device.registerDevice(fmt::format(simpleRootDesc, deviceInfo.udn, deviceInfo.location), deviceInfo);

    device.ControlActionRequested2 = [&] (auto& arg) { return mock.onActionRequest(arg); };

    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });
    action.addArgument("Channel", "Master");
    action.addArgument("DesiredVolume", "49");
    action.addArgument("InstanceID", "0");

    std::promise<void> prom;
    auto fut = prom.get_future();

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Invoke([&] (const ActionRequest& req) {
        EXPECT_EQ("urn:schemas-upnp-org:service:RenderingControl:1", req.serviceType);
        EXPECT_EQ("SetVolume"s, req.actionName);

        return s_response;
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

    fut.wait();
    client.uninitialize();
    device.uninitialize();
}

}
}
