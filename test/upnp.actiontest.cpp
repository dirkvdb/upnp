#include <gtest/gtest.h>

#include "upnp.action.cpp"

namespace upnp
{
namespace test
{

using namespace std::literals::string_literals;
using namespace testing;

TEST(UpnpActionTest, CreateAction)
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

    EXPECT_EQ("SetVolume", action.getName());
    EXPECT_EQ("url", action.getUrl());
    EXPECT_EQ(ServiceType::RenderingControl, action.getServiceType());
    EXPECT_EQ("urn:schemas-upnp-org:service:RenderingControl:1", action.getServiceTypeUrn());
    EXPECT_EQ(expected, action.toString());
}

TEST(UpnpActionTest, CreateActionNoArguments)
{
    auto expected =
    "<?xml version=\"1.0\"?>"
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
    "<s:Body>"
    "<u:SetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\"/>"
    "</s:Body>"
    "</s:Envelope>"s;

    Action action("SetVolume", "url", ServiceType::RenderingControl);

    EXPECT_EQ("SetVolume", action.getName());
    EXPECT_EQ("url", action.getUrl());
    EXPECT_EQ(ServiceType::RenderingControl, action.getServiceType());
    EXPECT_EQ("urn:schemas-upnp-org:service:RenderingControl:1", action.getServiceTypeUrn());
    EXPECT_EQ(expected, action.toString());
}

}
}
