#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp.ssdp.parseutils.h"

#include "upnp.http.parser.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::literals::string_literals;

class SsdpTest : public Test
{
public:
    ssdp::DeviceNotificationInfo info;
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

//TEST_CASE("PARSE notify", "[parse]")
//{
//    auto notification =
//        "NOTIFY * HTTP/1.1\r\n"
//        "HOST: 239.255.255.250:1900\r\n"
//        "CACHE-CONTROL: max-age=60\r\n"
//        "LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
//        "SERVER: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
//        "NT: urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "USN: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "NTS: ssdp:alive\r\n"
//        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//        "01-NLS: 1\r\n"
//        "BOOTID.UPNP.ORG: 1\r\n"
//        "CONFIGID.UPNP.ORG: 1337\r\n\r\n"s;
//
//    auto info = ssdp::parseNotification(notification);
//    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
//    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
//    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
//    CHECK(info.expirationTime == 60);
//    CHECK(info.type == ssdp::NotificationType::Alive);
//}
//
//TEST_CASE("PARSE notify no spaces", "[parse]")
//{
//    auto notification =
//        "NOTIFY * HTTP/1.1\r\n"
//        "HOST:239.255.255.250:1900\r\n"
//        "CACHE-CONTROL:max-age=60\r\n"
//        "LOCATION:http://192.168.1.1:5000/rootDesc.xml\r\n"
//        "SERVER:Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
//        "NT:urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "USN:uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "NTS:ssdp:alive\r\n"
//        "OPT:\"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//        "01-NLS:1\r\n"
//        "BOOTID.UPNP.ORG:1\r\n"
//        "CONFIGID.UPNP.ORG:1337\r\n\r\n"s;
//
//    auto info = ssdp::parseNotification(notification);
//    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
//    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
//    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
//    CHECK(info.expirationTime == 60);
//    CHECK(info.type == ssdp::NotificationType::Alive);
//}
//
//TEST_CASE("PARSE notify lowercase fields", "[parse]")
//{
//    auto notification =
//        "NOTIFY * HTTP/1.1\r\n"
//        "host: 239.255.255.250:1900\r\n"
//        "cache-control: max-age=60\r\n"
//        "location: http://192.168.1.1:5000/rootDesc.xml\r\n"
//        "server: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
//        "nt: urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "usn: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "nts: ssdp:alive\r\n"
//        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//        "01-nls: 1\r\n"
//        "bootid.upnp.org: 1\r\n"
//        "configid.upnp.org: 1337\r\n\r\n"s;
//
//    auto info = ssdp::parseNotification(notification);
//    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
//    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
//    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
//    CHECK(info.expirationTime == 60);
//    CHECK(info.type == ssdp::NotificationType::Alive);
//}
//
//TEST_CASE("PARSE notify byebye", "[parse]")
//{
//    auto notification =
//        "NOTIFY * HTTP/1.1\r\n"
//        "HOST: 239.255.255.250:1900\r\n"
//        "CACHE-CONTROL: max-age=180\r\n"
//        "LOCATION: http://192.168.1.219:49155/description.xml\r\n"
//        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//        "01-NLS: b0fcf5a0-f417-11e5-96c9-faa940f8bedc\r\n"
//        "NT: urn:schemas-upnp-org:service:ConnectionManager:1\r\n"
//        "NTS: ssdp:byebye\r\n"
//        "SERVER: Darwin/15.4.0, UPnP/1.0, Portable SDK for UPnP devices/1.6.19\r\n"
//        "X-User-Agent: redsonic\r\n"
//        "USN: uuid:356a6e90-8e58-11e2-9e96-0800200c9a55::urn:schemas-upnp-org:service:ConnectionManager:1\r\n\r\n"s;
//
//    auto info = ssdp::parseNotification(notification);
//    CHECK(info.location == "http://192.168.1.219:49155/description.xml");
//    CHECK(info.deviceId == "uuid:356a6e90-8e58-11e2-9e96-0800200c9a55");
//    CHECK(info.deviceType == "urn:schemas-upnp-org:service:ConnectionManager:1");
//    CHECK(info.expirationTime == 180);
//    CHECK(info.type == ssdp::NotificationType::ByeBye);
//}
//
//TEST_CASE("PARSE notify invalid type", "[parse]")
//{
//    auto notification =
//        "NOTIFY * HTTP/1.1\r\n"
//        "HOST: 239.255.255.250:1900\r\n"
//        "CACHE-CONTROL: max-age=60\r\n"
//        "LOCATION: http://192.168.1.1:5000/rootDesc.xml\r\n"
//        "SERVER: Compal Broadband Networks, Inc/Linux/2.6.39.3 UPnP/1.1 MiniUPnPd/1.7\r\n"
//        "NT: urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "USN: uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:WANDevice:1\r\n"
//        "NTS: ssdp::alive\r\n"
//        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//        "01-NLS: 1\r\n"
//        "BOOTID.UPNP.ORG: 1\r\n"
//        "CONFIGID.UPNP.ORG: 1337\r\n\r\n"s;
//
//    CHECK_THROWS_AS(ssdp::parseNotification(notification), std::runtime_error);
//}

}
}
