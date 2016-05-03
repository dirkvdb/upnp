#include <gmock/gmock.h>
#include <sstream>

#include "utils/log.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"

#include "testxmls.h"
#include "testutils.h"
#include "rapidxml_print.hpp"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace rapidxml_ns;

static const std::string serviceDesc =
"<?xml version=\"1.0\"?>"
"<scpd"
"  xmlns=\"urn:schemas-upnp-org:service-1-0\">"
"    <specVersion>"
"        <major>1</major>"
"        <minor>0</minor>"
"    </specVersion>"
"    <actionList>"
"        <action>"
"            <name>GetVolume</name>"
"            <argumentList>"
"                <argument>"
"                    <name>InstanceID</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>Channel</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>CurrentVolume</name>"
"                    <direction>out</direction>"
"                    <relatedStateVariable>Volume</relatedStateVariable>"
"                </argument>"
"            </argumentList>"
"        </action>"
"        <action>"
"            <name>SetVolume</name>"
"            <argumentList>"
"                <argument>"
"                    <name>InstanceID</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>Channel</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>DesiredVolume</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>Volume</relatedStateVariable>"
"                </argument>"
"            </argumentList>"
"        </action>"
"    </actionList>"
"    <serviceStateTable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>PresetNameList</name>"
"            <dataType>string</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"yes\">"
"            <name>LastChange</name>"
"            <dataType>string</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>Mute</name>"
"            <dataType>boolean</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>Volume</name>"
"            <dataType>ui2</dataType>"
"            <allowedValueRange>"
"                <minimum>0</minimum>"
"                <maximum>100</maximum>"
"                <step>1</step>"
"            </allowedValueRange>"
"        </stateVariable>"
"    </serviceStateTable>"
"</scpd>";

TEST(XmlParseTest, Decode)
{
    EXPECT_EQ("\"<'&'>\"", xml::decode("&quot;&lt;&apos;&amp;&apos;&gt;&quot;"));
    EXPECT_EQ("&", xml::decode("&"));
    EXPECT_EQ("&&", xml::decode("&&"));
    EXPECT_EQ("&&&", xml::decode("&&&"));
    EXPECT_EQ("&&&&", xml::decode("&&&&"));
    EXPECT_EQ("&&&&&", xml::decode("&&&&&"));
    EXPECT_EQ("&&&&&&", xml::decode("&&&&&&"));

    EXPECT_EQ("...&quot", xml::decode("...&quot"));
    EXPECT_EQ("...&quo", xml::decode("...&quo"));
    EXPECT_EQ("...&qu", xml::decode("...&qu"));
    EXPECT_EQ("...&q", xml::decode("...&q"));

    EXPECT_EQ("...&lt", xml::decode("...&lt"));
    EXPECT_EQ("...&l", xml::decode("...&l"));

    EXPECT_EQ("...&gt", xml::decode("...&gt"));
    EXPECT_EQ("...&g", xml::decode("...&g"));

    EXPECT_EQ("...&apos", xml::decode("...&apos"));
    EXPECT_EQ("...&apo", xml::decode("...&apo"));
    EXPECT_EQ("...&ap", xml::decode("...&ap"));
    EXPECT_EQ("...&a", xml::decode("...&a"));

    EXPECT_EQ("...&amp", xml::decode("...&amp"));
    EXPECT_EQ("...&am", xml::decode("...&am"));
    EXPECT_EQ("...&a", xml::decode("...&a"));

    EXPECT_EQ("...&amp&&amp", xml::decode("...&amp&amp;&amp"));
    EXPECT_EQ("", xml::decode(""));
    EXPECT_EQ("<hello attr=\"value\">", xml::decode("&lt;hello attr=&quot;value&quot;&gt;"));
    EXPECT_EQ("Hello&", xml::decode("Hello&"));
}

TEST(XmlParseTest, Encode)
{
    EXPECT_EQ("&quot;&lt;&apos;&amp;&apos;&gt;&quot;", xml::encode("\"<'&'>\""));
    EXPECT_EQ("&amp;", xml::encode("&"));
    EXPECT_EQ("&amp;&amp;", xml::encode("&&"));

    EXPECT_EQ("&amp;amp;", xml::encode("&amp;"));
    EXPECT_EQ("", xml::encode(""));
    EXPECT_EQ("&lt;hello attr=&quot;value&quot;&gt;", xml::encode("<hello attr=\"value\">"));
    EXPECT_EQ("Hello&amp;", xml::encode("Hello&"));
}

TEST(XmlParseTest, GatewayDeviceInfo)
{
    Device dev;
    dev.m_location = "http://192.168.1.1:5000/rootDesc.xml";

    xml::parseDeviceInfo(testxmls::gatewayRootDesc, dev);

    EXPECT_EQ("uuid:A37351C5-8521-4c24-A43E-5C353B9982A9"s, dev.m_udn);
    EXPECT_EQ("Compal Broadband Networks, Inc CH6643"s, dev.m_friendlyName);
    EXPECT_TRUE(dev.m_baseURL.empty());
    EXPECT_EQ(DeviceType::InternetGateway, dev.m_type);
    EXPECT_TRUE(dev.m_presURL.empty());
}

TEST(XmlParseTest, MediaServerInfo)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    xml::parseDeviceInfo(testxmls::mediaServerRootDesc, dev);

    EXPECT_EQ("uuid:55076f6e-6b79-1d65-a4eb-00089be34071"s, dev.m_udn);
    EXPECT_EQ("NAS"s, dev.m_friendlyName);
    EXPECT_TRUE(dev.m_baseURL.empty());
    EXPECT_EQ(DeviceType::MediaServer, dev.m_type);
    EXPECT_EQ("http://192.168.1.13:9000/"s, dev.m_presURL);

    EXPECT_EQ(2u, dev.m_services.size());

    auto& connMgrSvc = dev.m_services.at(ServiceType::ConnectionManager);
    EXPECT_EQ(ServiceType::ConnectionManager, connMgrSvc.m_type);
    EXPECT_EQ("urn:upnp-org:serviceId:ConnectionManager"s, connMgrSvc.m_id);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv0/control"s, connMgrSvc.m_controlURL);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv0/event"s, connMgrSvc.m_eventSubscriptionURL);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv0.xml"s, connMgrSvc.m_scpdUrl);

    auto& contDirMgrSvc = dev.m_services.at(ServiceType::ContentDirectory);
    EXPECT_EQ(ServiceType::ContentDirectory, contDirMgrSvc.m_type);
    EXPECT_EQ("urn:upnp-org:serviceId:ContentDirectory"s, contDirMgrSvc.m_id);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv1/control"s, contDirMgrSvc.m_controlURL);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv1/event"s, contDirMgrSvc.m_eventSubscriptionURL);
    EXPECT_EQ("http://192.168.1.13:9000/dev0/srv1.xml"s, contDirMgrSvc.m_scpdUrl);
}

TEST(XmlParseTest, EmptyXml)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    EXPECT_THROW(xml::parseDeviceInfo("", dev), std::runtime_error);
}

TEST(XmlParseTest, MissingFriendlyName)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    auto xml = testxmls::gatewayRootDesc;
    xml_document<> doc;
    doc.parse<parse_default>(&xml.front());
    doc.first_node_ref("root").first_node_ref("device").remove_node("friendlyName");

    std::stringstream xmlMod;
    xmlMod << doc;
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, MissingUdn)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    auto xml = testxmls::gatewayRootDesc;
    xml_document<> doc;
    doc.parse<parse_non_destructive>(xml.c_str());
    doc.first_node_ref("root").first_node_ref("device").remove_node("UDN");

    std::stringstream xmlMod;
    xmlMod << *doc.first_node();
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, MissingDeviceType)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    xml_document<> doc;
    doc.parse<parse_non_destructive>(testxmls::gatewayRootDesc.c_str());
    doc.first_node_ref("root").first_node_ref("device").remove_node("deviceType");

    std::stringstream xmlMod;
    xmlMod << *doc.first_node();
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, ParseServiceDescription)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive>(serviceDesc.c_str());

    struct ActionMock
    {
        MOCK_METHOD1(onAction, void(const std::string&));
    };

    ActionMock mock;
    EXPECT_CALL(mock, onAction("GetVolume"s));
    EXPECT_CALL(mock, onAction("SetVolume"s));

    auto vars = xml::parseServiceDescription(serviceDesc, [&] (const std::string& action) {
        mock.onAction(action);
    });

    auto iter = std::find_if(vars.begin(), vars.end(), [] (const StateVariable& var) {
        return var.name == "LastChange";
    });

    ASSERT_NE(vars.end(), iter);
    EXPECT_STREQ("LastChange", iter->name.c_str());
    EXPECT_STREQ("string", iter->dataType.c_str());
    EXPECT_EQ(nullptr, iter->valueRange);

    iter = std::find_if(vars.begin(), vars.end(), [] (const StateVariable& var) {
        return var.name == "Volume";
    });

    ASSERT_NE(vars.end(), iter);
    EXPECT_STREQ("Volume", iter->name.c_str());
    EXPECT_STREQ("ui2", iter->dataType.c_str());
    EXPECT_NE(nullptr, iter->valueRange);
    EXPECT_EQ(0, iter->valueRange->minimumValue);
    EXPECT_EQ(100, iter->valueRange->maximumValue);
    EXPECT_EQ(1U, iter->valueRange->step);
}

static const std::string eventDesc =
"<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\">"
"  <InstanceID val=\"0\">"
"    <TransportState val=\"PLAYING\"/>"
"    <CurrentTrackURI val=\"http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2598668.mp3\"/>"
"    <CurrentTransportActions val=\"Pause,Stop,Next,Previous\"/>"
"  </InstanceID>"
"</Event>";

TEST(XmlParseTest, GetEventValues)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive>(eventDesc.c_str());

    auto values = xml::getEventValues(doc);
    EXPECT_EQ(3u, values.size());
    EXPECT_EQ("PLAYING"s, values.at("TransportState"));
    EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2598668.mp3"s, values.at("CurrentTrackURI"));
    EXPECT_EQ("Pause,Stop,Next,Previous"s, values.at("CurrentTransportActions"));
}

TEST(XmlParseTest, ParseBrowseResponseItems)
{
    ContentDirectory::ActionResult result;
    auto didlLite = xml::parseBrowseResult(wrapSoap(testxmls::browseResponseItems), result);

    EXPECT_EQ(2u, result.numberReturned);
    EXPECT_EQ(12u, result.totalMatches);
    EXPECT_EQ(2u, result.updateId);

    auto items = xml::parseItems(xml::decode(didlLite));
    EXPECT_EQ(2u, items.size());

    for (auto& item : items)
    {
        EXPECT_EQ(1u, item.getAlbumArtUris().size());
        EXPECT_EQ(1u, item.getResources().size());

        auto& artUri = item.getAlbumArtUris().at(dlna::ProfileId::JpegThumbnail);
        auto& res = item.getResources().at(0);

        if (item.getObjectId() == "0$1$12$38502R2290700")
        {
            EXPECT_EQ("0$1$8I2290700"s, item.getRefId());
            EXPECT_TRUE(item.restricted());
            EXPECT_EQ("Momove"s, item.getTitle());
            EXPECT_EQ("1", item.getMetaData(Property::TrackNumber));

            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2290700.jpg?scale=org"s, artUri);
            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2290700.mp3"s, res.getUrl());
            EXPECT_EQ(10053760u, res.getSize());
            EXPECT_EQ(24000u, res.getBitRate());
            EXPECT_EQ(std::chrono::seconds(6*60 + 54), res.getDuration());
        }
        else if (item.getObjectId() == "0$1$12$38502R2291724")
        {
            EXPECT_EQ("0$1$8I2291724"s, item.getRefId());
            EXPECT_FALSE(item.restricted());
            EXPECT_EQ("Two Months Off"s, item.getTitle());
            EXPECT_EQ("2", item.getMetaData(Property::TrackNumber));

            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2291724.jpg?scale=org"s, artUri);
            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291724.mp3"s, res.getUrl());
            EXPECT_EQ(13275264u, res.getSize());
            EXPECT_EQ(24000u, res.getBitRate());
            EXPECT_EQ(std::chrono::seconds(9*60 + 9), res.getDuration());
        }
        else
        {
            FAIL() << "Unexpected item in results: " << item.getObjectId();
        }

        // Common values
        EXPECT_EQ(0u, item.getChildCount());
        EXPECT_EQ("0$1$12$38502"s, item.getParentId());
        EXPECT_EQ("2002-01-01"s, item.getMetaData(Property::Date));
        EXPECT_EQ("Club"s, item.getMetaData(Property::Genre));
        EXPECT_EQ("A Hundred Days Off"s, item.getMetaData(Property::Album));
        EXPECT_EQ("Underworld"s, item.getMetaData(Property::Creator));
        EXPECT_EQ("Underworld"s, item.getMetaData(Property::Artist));
        EXPECT_EQ("Underworld"s, item.getMetaData(Property::AlbumArtist));
        EXPECT_EQ(Class::Audio, item.getClass());
    }
}

TEST(XmlParseTest, ParseBrowseResponseContainers)
{
    ContentDirectory::ActionResult result;
    auto didlLite = xml::parseBrowseResult(wrapSoap(testxmls::browseResponseContainers), result);

    EXPECT_EQ(2u, result.numberReturned);
    EXPECT_EQ(3u, result.totalMatches);
    EXPECT_EQ(4u, result.updateId);

    auto items = xml::parseContainers(xml::decode(didlLite));
    EXPECT_EQ(2u, items.size());

    for (auto& item : items)
    {
        EXPECT_TRUE(item.getResources().empty());
        EXPECT_EQ(1u, item.getAlbumArtUris().size());
        auto& artUri = item.getAlbumArtUris().at(dlna::ProfileId::JpegThumbnail);

        if (item.getObjectId() == "0$1$17$994$23308")
        {
            EXPECT_EQ(12u, item.getChildCount());
            EXPECT_TRUE(item.restricted());
            EXPECT_EQ("+DOME"s, item.getTitle());
            EXPECT_EQ("Dubstep, Indie, Experimental, Ambient"s, item.getMetaData(Property::Genre));
            EXPECT_EQ("+DOME"s, item.getMetaData(Property::Album));
            EXPECT_EQ("Seekae"s, item.getMetaData(Property::Creator));
            EXPECT_EQ("Seekae"s, item.getMetaData(Property::Artist));
            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/C/O0$1$17$994$23308.jpg?scale=org"s, artUri);
        }
        else if (item.getObjectId() == "0$1$17$994$20156")
        {
            EXPECT_EQ(11u, item.getChildCount());
            EXPECT_FALSE(item.restricted());
            EXPECT_EQ("My Favorite Things"s, item.getTitle());
            EXPECT_EQ("Broken Beat, Deep House, Experimental, Ambient"s, item.getMetaData(Property::Genre));
            EXPECT_EQ("My Favorite Things"s, item.getMetaData(Property::Album));
            EXPECT_EQ("Toshiya Kawasaki"s, item.getMetaData(Property::Creator));
            EXPECT_EQ("Toshiya Kawasaki"s, item.getMetaData(Property::Artist));
            EXPECT_EQ(Class::AudioContainer, item.getClass());
            EXPECT_EQ("http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/C/O0$1$17$994$20156.jpg?scale=org"s, artUri);
        }
        else
        {
            FAIL() << "Unexpected item in results: " << item.getObjectId();
        }

        EXPECT_EQ("0$1$17$994"s, item.getParentId());
        EXPECT_EQ(""s, item.getRefId());
        EXPECT_EQ(Class::AudioContainer, item.getClass());
    }
}

}
}
