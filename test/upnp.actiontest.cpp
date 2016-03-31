#include <catch.hpp>

#include "upnp.action.cpp"

namespace upnp
{
namespace test
{

using namespace std::literals::string_literals;

TEST_CASE("Create action", "[XML]")
{
    auto expected =
    "<?xml version=\"1.0\"?>"
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body>"
    "<u:SetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">"
    "<Channel>Master</Channel>"
    "<DesiredVolume>49</DesiredVolume>"
    "<InstanceID>0</InstanceID>"
    "</u:SetVolume>"
    "</s:Body>"
    "</s:Envelope>"s;
    
    Action action("SetVolume", "url", ServiceType::RenderingControl);
    action.addArgument("Channel", "Master");
    action.addArgument("DesiredVolume", "49");
    action.addArgument("InstanceID", "0");
    
    CHECK(action.getName() == "SetVolume");
    CHECK(action.getUrl() == "url");
    CHECK(action.getServiceType() == ServiceType::RenderingControl);
    CHECK(action.getServiceTypeUrn() == "urn:schemas-upnp-org:service:RenderingControl:1");
    CHECK(action.toString() == expected);
}

}
}
