#include <catch.hpp>

#include "utils/log.h"
#include "upnp.ssdp.parseutils.h"

namespace upnp
{
namespace test
{

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
        CHECK_THROWS_AS(parseUSN("uuid:A37351C5-8521-4c24-A43E-5C353B9982A", info), std::runtime_error);
    }
}

}
}
