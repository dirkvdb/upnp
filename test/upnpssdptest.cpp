#include <catch.hpp>

#include "utils/log.h"
#include "upnp.ssdp.parseutils.h"

#include "upnphttpparser.h"

namespace upnp
{
namespace test
{

using namespace std::literals::string_literals;

TEST_CASE("Parse cache control", "[Parse]")
{
    CHECK(60 == ssdp::parseCacheControl("max-age=60"));
    CHECK(0 == ssdp::parseCacheControl("max-age=0"));

    CHECK_THROWS_AS(ssdp::parseCacheControl("Max-age=0"), std::runtime_error);
    CHECK_THROWS_AS(ssdp::parseCacheControl(""), std::runtime_error);
    CHECK_THROWS_AS(ssdp::parseCacheControl("max-age=five"), std::runtime_error);
    CHECK_THROWS_AS(ssdp::parseCacheControl("max-age=5.5"), std::runtime_error);
    CHECK_THROWS_AS(ssdp::parseCacheControl("max-age=-0"), std::runtime_error);
    CHECK_THROWS_AS(ssdp::parseCacheControl("max-age=55555555555555555555555555555555"), std::out_of_range);
}

TEST_CASE("PARSE USN", "[Parse]")
{
    ssdp::DeviceDiscoverInfo info;

    SECTION("Parse")
    {
        parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9::urn:schemas-upnp-org:device:InternetGatewayDevice:1", info);
        CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
        CHECK(info.deviceType == "urn:schemas-upnp-org:device:InternetGatewayDevice:1");
    }

    SECTION("Parse no type")
    {
        parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9", info);
        CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
        CHECK(info.deviceType.empty());
    }

    SECTION("Parse invalid uuid")
    {
        CHECK_THROWS_AS(parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982AY", info), std::runtime_error);
        CHECK_THROWS_AS(parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A", info), std::runtime_error);
        CHECK_THROWS_AS(parseUSN("uuid:A37351C585214c24A43E5C353B9982A", info), std::runtime_error);
        CHECK_THROWS_AS(parseUSN("", info), std::runtime_error);
    }
}

TEST_CASE("PARSE notify", "[Parse]")
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

    auto info = ssdp::parseNotification(notification);
    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
    CHECK(info.expirationTime == 60);
}

TEST_CASE("PARSE notify no spaces", "[Parse]")
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

    auto info = ssdp::parseNotification(notification);
    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
    CHECK(info.expirationTime == 60);
}


TEST_CASE("PARSE notify lowercase fields", "[Parse]")
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

    auto info = ssdp::parseNotification(notification);
    CHECK(info.location == "http://192.168.1.1:5000/rootDesc.xml");
    CHECK(info.deviceId == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
    CHECK(info.deviceType == "urn:schemas-upnp-org:device:WANDevice:1");
    CHECK(info.expirationTime == 60);
}

TEST_CASE("PARSE invalid notify", "[Parse]")
{
    auto notification =
        "notify * HTTP/1.1\r\n"
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

    CHECK_THROWS_AS(ssdp::parseNotification(notification), std::runtime_error);
}

}
}
