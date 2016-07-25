#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>

#include "utils/log.h"
#include "upnp/upnp.ssdp.client.h"
#include "upnp/upnp.http.parser.h"
#include "upnp.ssdp.parseutils.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::placeholders;
using namespace std::literals::string_literals;

struct DeviceNotificationMock
{
    MOCK_METHOD1(onDevice, void(const ssdp::DeviceNotificationInfo&));
};

class SsdpTest : public Test
{
public:
    SsdpTest()
    {
        parser.setHeaderParsedCallback(std::bind(&DeviceNotificationMock::onDevice, &devMock, _1));
    }

    ssdp::DeviceNotificationInfo info;
    ssdp::Parser parser;
    DeviceNotificationMock devMock;
};

TEST_F(SsdpTest, ParseCacheControl)
{
    EXPECT_EQ(60u, ssdp::parseCacheControl("max-age=60"));
    EXPECT_EQ(0u, ssdp::parseCacheControl("max-age=0"));

    EXPECT_THROW(ssdp::parseCacheControl("Max-age=0"), std::runtime_error);
    EXPECT_THROW(ssdp::parseCacheControl(""), std::runtime_error);
    EXPECT_THROW(ssdp::parseCacheControl("max-age=five"), std::runtime_error);
    EXPECT_THROW(ssdp::parseCacheControl("max-age=5.5"), std::runtime_error);
    EXPECT_THROW(ssdp::parseCacheControl("max-age=-0"), std::runtime_error);
    EXPECT_THROW(ssdp::parseCacheControl("max-age=55555555555555555555555555555555"), std::out_of_range);
}

TEST_F(SsdpTest, ParseUsn)
{
    parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:InternetGatewayDevice:1", info);
    EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9", info.deviceId);
    EXPECT_EQ("urn:schemas-upnp-org:device:InternetGatewayDevice:1", info.deviceType);
}

TEST_F(SsdpTest, ParseUsnNoType)
{
    parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9", info);
    EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9", info.deviceId);
    EXPECT_TRUE(info.deviceType.empty());
}

TEST_F(SsdpTest, ParseInvalidUuid)
{
    EXPECT_THROW(parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982AY", info), std::runtime_error);
    EXPECT_THROW(parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A", info), std::runtime_error);
    EXPECT_THROW(parseUSN("uuid:A37351C585214c24A43E5C353B9982A", info), std::runtime_error);
    EXPECT_THROW(parseUSN("", info), std::runtime_error);
}

TEST_F(SsdpTest, ParseNotify)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "CACHE-CONTROL: max-age=60\r\n"
        "LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
        "SERVER: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
        "NT: urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "USN: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "NTS: ssdp:alive\r\n"
        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS: 1\r\n"
        "BOOTID.UPNP.ORG: 1\r\n"
        "CONFIGID.UPNP.ORG: 1337\r\n\r\n"s;

    EXPECT_CALL(devMock, onDevice(_)).WillOnce(Invoke([] (const ssdp::DeviceNotificationInfo& info) {
        EXPECT_EQ("http://192.168.1.1:5000/rootDesc.xml"s, info.location);
        EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9"s, info.deviceId);
        EXPECT_EQ("urn:schemas-upnp-org:device:WANDevice:1"s, info.deviceType);
        EXPECT_EQ(60u, info.expirationTime);
        EXPECT_EQ(ssdp::NotificationType::Alive, info.type);
    }));

    EXPECT_EQ(notification.size(), parser.parse(notification));
}

TEST_F(SsdpTest, ParseNotifyChunked)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "HOST:239.255.255.250:1900\r\n"
        "CACHE-CONTROL:max-age=60\r\n"
        "LOCATION:http://192.168.1.1:5000/rootDesc.xml\r\n"
        "SERVER:Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
        "NT:urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "USN:uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "NTS:ssdp:alive\r\n"
        "OPT:\"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS:1\r\n"
        "BOOTID.UPNP.ORG:1\r\n"
        "CONFIGID.UPNP.ORG:1337\r\n\r\n"s;

    EXPECT_EQ(notification.size() / 2, parser.parse(notification.substr(0, notification.size() / 2)));

    EXPECT_CALL(devMock, onDevice(_)).WillOnce(Invoke([] (const ssdp::DeviceNotificationInfo& info) {
        EXPECT_EQ("http://192.168.1.1:5000/rootDesc.xml"s, info.location);
        EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9"s, info.deviceId);
        EXPECT_EQ("urn:schemas-upnp-org:device:WANDevice:1"s, info.deviceType);
        EXPECT_EQ(60u, info.expirationTime);
        EXPECT_EQ(ssdp::NotificationType::Alive, info.type);
    }));

    EXPECT_EQ(notification.size() - (notification.size() / 2), parser.parse(notification.substr(notification.size() / 2)));
}

TEST_F(SsdpTest, ParseNotifyNoSpaces)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "HOST:239.255.255.250:1900\r\n"
        "CACHE-CONTROL:max-age=60\r\n"
        "LOCATION:http://192.168.1.1:5000/rootDesc.xml\r\n"
        "SERVER:Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
        "NT:urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "USN:uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "NTS:ssdp:alive\r\n"
        "OPT:\"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS:1\r\n"
        "BOOTID.UPNP.ORG:1\r\n"
        "CONFIGID.UPNP.ORG:1337\r\n\r\n"s;

    EXPECT_CALL(devMock, onDevice(_)).WillOnce(Invoke([] (const ssdp::DeviceNotificationInfo& info) {
        EXPECT_EQ("http://192.168.1.1:5000/rootDesc.xml"s, info.location);
        EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9"s, info.deviceId);
        EXPECT_EQ("urn:schemas-upnp-org:device:WANDevice:1"s, info.deviceType);
        EXPECT_EQ(60u, info.expirationTime);
        EXPECT_EQ(ssdp::NotificationType::Alive, info.type);
    }));

    EXPECT_EQ(notification.size(), parser.parse(notification));
}

TEST_F(SsdpTest, ParseNotifyLowercaseFields)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "host: 239.255.255.250:1900\r\n"
        "cache-control: max-age=60\r\n"
        "location: http://192.168.1.1:5000/rootDesc.xml\r\n"
        "server: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
        "nt: urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "usn: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "nts: ssdp:alive\r\n"
        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-nls: 1\r\n"
        "bootid.upnp.org: 1\r\n"
        "configid.upnp.org: 1337\r\n\r\n"s;

    EXPECT_CALL(devMock, onDevice(_)).WillOnce(Invoke([] (const ssdp::DeviceNotificationInfo& info) {
        EXPECT_EQ("http://192.168.1.1:5000/rootDesc.xml"s, info.location);
        EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9"s, info.deviceId);
        EXPECT_EQ("urn:schemas-upnp-org:device:WANDevice:1"s, info.deviceType);
        EXPECT_EQ(60u, info.expirationTime);
        EXPECT_EQ(ssdp::NotificationType::Alive, info.type);
    }));

    EXPECT_EQ(notification.size(), parser.parse(notification));
}

TEST_F(SsdpTest, ParseNotifyByeBye)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "LOCATION: http://192.168.1.219:49155/description.xml\r\n"
        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS: b0fcf5a0-f417-11e5-96c9-faa940f8bedc\r\n"
        "NT: urn:schemas-upnp-org:service:ConnectionManager:1\r\n"
        "NTS: ssdp:byebye\r\n"
        "SERVER: Darwin/15.4.0, UPnP/1.0, Portable SDK for UPnP devices/1.6.19\r\n"
        "X-User-Agent: redsonic\r\n"
        "USN: uuid:356a6e90-8e58-11e2-9e96-0800200c9a55::urn:schemas-upnp-org:service:ConnectionManager:1\r\n\r\n"s;

    EXPECT_CALL(devMock, onDevice(_)).WillOnce(Invoke([] (const ssdp::DeviceNotificationInfo& info) {
        EXPECT_EQ("http://192.168.1.219:49155/description.xml"s, info.location);
        EXPECT_EQ("uuid:356a6e90-8e58-11e2-9e96-0800200c9a55"s, info.deviceId);
        EXPECT_EQ("urn:schemas-upnp-org:service:ConnectionManager:1"s, info.deviceType);
        EXPECT_EQ(0u, info.expirationTime);
        EXPECT_EQ(ssdp::NotificationType::ByeBye, info.type);
    }));

    EXPECT_EQ(notification.size(), parser.parse(notification));
}

TEST_F(SsdpTest, ParseNotifyInvalidType)
{
    auto notification =
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "CACHE-CONTROL: max-age=60\r\n"
        "LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
        "SERVER: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
        "NT: urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "USN: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
        "NTS: ssdp::alive\r\n"
        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS: 1\r\n"
        "BOOTID.UPNP.ORG: 1\r\n"
        "CONFIGID.UPNP.ORG: 1337\r\n\r\n"s;

    EXPECT_CALL(devMock, onDevice(_)).Times(0);
    EXPECT_EQ(notification.size(), parser.parse(notification));
}

}
}
