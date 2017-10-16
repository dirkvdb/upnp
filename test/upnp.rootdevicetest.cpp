#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp.client.h"
#include "upnp/upnp.rootdevice.h"
#include "upnp/upnp.asio.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.devicescanner.h"
#include "upnp/upnp.soap.types.h"
#include "upnp/upnp.servicefaults.h"
#include "upnp.soap.parseutils.h"
#include "upnp.soap.client.h"

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
    "    <specVersion>"
    "        <major>1</major>"
    "        <minor>0</minor>"
    "    </specVersion>"
    "   <device>"
    "       <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>"
    "       <UDN>{}</UDN>"
    "       <friendlyName>Test Device</friendlyName>"
    "       <UPC></UPC>"
    "       <presentationURL>{}</presentationURL>"
    "       <serviceList/>"
    "   </device>"
    "</root>";

struct DeviceCallbackMock
{
    MOCK_METHOD1(onActionRequest, std::string(const ActionRequest&));
    MOCK_METHOD1(onSubscriptionRequest, SubscriptionResponse(const SubscriptionRequest&));

    MOCK_METHOD1(onEvent, void(const SubscriptionEvent&));
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
        device.EventSubscriptionRequested = [&] (auto& arg) { return mock.onSubscriptionRequest(arg); };

        client.initialize(getLoopbackInterface(), 0);
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
            try
            {
                scanner.getDevice(device.getUniqueDeviceName());
            }
            catch (std::invalid_argument)
            {
                // Device still not discovered, try one more wait
                scanner.refresh();
                EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
            }
        }
    }

    Client client;
    Device deviceInfo;
    DeviceCallbackMock mock;
    RootDevice device;
};

TEST_F(RootDeviceTest, SubscriptionRequest)
{
    std::string generatedSid;
    std::promise<void> prom;
    auto fut = prom.get_future();

    EXPECT_CALL(mock, onSubscriptionRequest(_)).WillOnce(Invoke([&] (const SubscriptionRequest& req) {
        EXPECT_EQ("/ctrl/rcev"s, req.url);
        EXPECT_THAT(req.sid, StartsWith("uuid:"));
        EXPECT_EQ(180s, req.timeout);

        generatedSid = req.sid;

        SubscriptionResponse res;
        res.timeout = 120s;
        res.initialEvent = "EventData"s;
        return res;
    }));

    EXPECT_CALL(mock, onEvent(_)).WillOnce(Invoke([&] (const SubscriptionEvent& ev) {
        EXPECT_EQ(generatedSid, ev.sid);
        EXPECT_EQ("EventData"s, ev.data);
        EXPECT_EQ(0u, ev.sequence);
        prom.set_value();
    }));

    client.subscribeToService(device.getWebrootUrl() + "/ctrl/rcev", 180s, [&] (Status s, std::string subId, std::chrono::seconds timeout) {
        EXPECT_TRUE(s);
        EXPECT_EQ(generatedSid, subId);
        EXPECT_EQ(120s, timeout);

        return [this] (SubscriptionEvent ev) { mock.onEvent(ev); };
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

TEST_F(RootDeviceTest, SubscriptionRequestInvalidCallback)
{
    std::promise<void> prom;
    auto fut = prom.get_future();

    client.ioService().post([&] () {
        auto soap = std::make_shared<soap::Client>(client.ioService());
        soap->subscribe(device.getWebrootUrl() + "/ctrl/rcev", "", 180s, [&prom, soap] (const std::error_code& error, http::StatusCode status, std::string, std::chrono::seconds) {
            EXPECT_FALSE(error);
            EXPECT_EQ(http::StatusCode::PreconditionFailed, status);
            prom.set_value();
        });
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

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
        EXPECT_EQ(http::StatusCode::Ok, actionResult.httpStatus);
        EXPECT_FALSE(actionResult.isFaulty());
        EXPECT_EQ("Success", actionResult.response);
        prom.set_value();
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

TEST_F(RootDeviceTest, ControlActionUnknownException)
{
    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Throw(std::runtime_error("")));

    std::promise<void> prom;
    auto fut = prom.get_future();
    client.sendAction(action, [&] (Status s, soap::ActionResult actionResult) {
        EXPECT_FALSE(s);
        EXPECT_EQ(http::StatusCode::InternalServerError, actionResult.httpStatus);
        EXPECT_TRUE(actionResult.isFaulty());
        EXPECT_EQ(ActionFailed(), actionResult.getFault());
        prom.set_value();
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

TEST_F(RootDeviceTest, ControlActionWithFault)
{
    Action action("SetVolume", device.getWebrootUrl() + "/ctrl/rc", { ServiceType::RenderingControl, 1 });

    EXPECT_CALL(mock, onActionRequest(_)).WillOnce(Throw(InvalidAction()));

    std::promise<void> prom;
    auto fut = prom.get_future();
    client.sendAction(action, [&] (Status s, soap::ActionResult actionResult) {
        EXPECT_FALSE(s);
        EXPECT_EQ(http::StatusCode::InternalServerError, actionResult.httpStatus);
        EXPECT_TRUE(actionResult.isFaulty());
        EXPECT_EQ(InvalidAction(), actionResult.getFault());
        prom.set_value();
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(3s));
}

}
}
