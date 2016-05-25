
#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.ssdp.client.h"
#include "upnp/upnp.ssdp.server.h"

#include "upnp/upnp.device.h"

namespace upnp
{
namespace test
{

using namespace std::chrono_literals;
using namespace testing;

class SsdpServerTest : public Test
{
public:
    SsdpServerTest()
    : server(loop)
    , client(loop)
    {
        Device dev;
        dev.location = "http://localhost/";
        dev.type = DeviceType(DeviceType::MediaRenderer, 2);
        dev.udn = "uuid:55076f6e-6b79-1d65-a4eb-00089be34072";

        Service svc;

        svc.type = ServiceType(ServiceType::ConnectionManager, 1);
        dev.services.emplace(svc.type.type, svc);

        svc.type = ServiceType(ServiceType::RenderingControl, 2);
        dev.services.emplace(svc.type.type, svc);

        server.run(dev);

        for (auto& intf : uv::getNetworkInterfaces())
        {
            if (intf.isIpv4() && !intf.isInternal)
            {
                localIp = intf.ipName();
                break;
            }
        }

        EXPECT_FALSE(localIp.empty()) << "Failed to obtain local ip address";
    }

    uv::Loop loop;
    ssdp::Server server;
    ssdp::Client client;
    std::string localIp;
};

TEST_F(SsdpServerTest, SearchRootDeviceMulticast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("upnp:rootdevice");

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchRootDeviceUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("upnp:rootdevice", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchAllUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("ssdp:all", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchDeviceTypeUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("urn:schemas-upnp-org:device:MediaRenderer:2", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchDeviceTypeSmallerVersionUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("urn:schemas-upnp-org:device:MediaRenderer:1", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchServiceUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("urn:schemas-upnp-org:service:RenderingControl:2", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchServiceSmallerVersionUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("urn:schemas-upnp-org:service:RenderingControl:1", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, SearchDeviceUuidVersionUnicast)
{
    client.setDeviceNotificationCallback([&] (auto&& msg) {
        if (msg.deviceId == "uuid:55076f6e-6b79-1d65-a4eb-00089be34072")
        {
            uv::stopLoopAndCloseRequests(loop);
        }
    });

    client.run();
    client.search("uuid:55076f6e-6b79-1d65-a4eb-00089be34072", localIp.c_str());

    loop.run(uv::RunMode::Default);
}

}
}
