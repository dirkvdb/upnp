#include <gtest/gtest.h>
#include <sstream>

#include "utils/log.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"

#include "testxmls.h"
#include "rapidxml_print.hpp"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace rapidxml_ns;

static const std::string mediaServerRootDesc =
"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
"   <specVersion>"
"       <major>1</major>"
"       <minor>0</minor>"
"   </specVersion>"
"   <device>"
"       <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMS-1.50</dlna:X_DLNADOC>"
"       <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">M-DMS-1.50</dlna:X_DLNADOC>"
"       <pv:extension xmlns:pv=\"http://www.pv.com/pvns/\"></pv:extension><pv:platformID xmlns:pv=\"http://www.pv.com/pvns/\">Linux 3.4.6; armv5tel</pv:platformID><deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>"
"       <UDN>uuid:55076f6e-6b79-1d65-a4eb-00089be34071</UDN>"
"       <friendlyName>NAS</friendlyName>"
"       <manufacturer>PacketVideo</manufacturer>"
"       <manufacturerURL>http://www.pv.com</manufacturerURL>"
"       <modelName>TwonkyServer</modelName>"
"       <modelURL>http://www.twonky.com</modelURL>"
"       <modelDescription>TwonkyServer (Linux, T2)</modelDescription>"
"       <modelNumber>8.2</modelNumber>"
"       <serialNumber>8.2</serialNumber>"
"       <UPC></UPC>"
"       <presentationURL>http://192.168.1.13:9000/</presentationURL>"
"       <iconList>"
"           <icon>"
"               <mimetype>image/jpeg</mimetype>"
"               <height>48</height>"
"               <width>48</width>"
"               <depth>24</depth>"
"               <url>/images/twonkyicon-48x48.jpg</url>"
"           </icon>"
"           <icon>"
"               <mimetype>image/jpeg</mimetype>"
"               <height>120</height>"
"               <width>120</width>"
"               <depth>24</depth>"
"               <url>/images/twonkyicon-120x120.jpg</url>"
"           </icon>"
"           <icon>"
"               <mimetype>image/png</mimetype>"
"               <height>48</height>"
"               <width>48</width>"
"               <depth>24</depth>"
"               <url>/images/twonkyicon-48x48.png</url>"
"           </icon>"
"           <icon>"
"               <mimetype>image/png</mimetype>"
"               <height>120</height>"
"               <width>120</width>"
"               <depth>24</depth>"
"               <url>/images/twonkyicon-120x120.png</url>"
"           </icon>"
"       </iconList>"
"       <serviceList>"
"           <service>"
"               <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>"
"               <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>"
"               <SCPDURL>/dev0/srv0.xml</SCPDURL>"
"               <eventSubURL>/dev0/srv0/event</eventSubURL>"
"               <controlURL>/dev0/srv0/control</controlURL>"
"           </service>"
"           <service>"
"               <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>"
"               <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>"
"               <SCPDURL>/dev0/srv1.xml</SCPDURL>"
"               <eventSubURL>/dev0/srv1/event</eventSubURL>"
"               <controlURL>/dev0/srv1/control</controlURL>"
"           </service>"
"           <service>"
"               <serviceType>urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1</serviceType>"
"               <serviceId>urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar</serviceId>"
"               <SCPDURL>/dev0/srv2.xml</SCPDURL>"
"               <eventSubURL>/dev0/srv2/event</eventSubURL>"
"               <controlURL>/dev0/srv2/control</controlURL>"
"           </service>"
"       </serviceList>"
"   </device>"
"</root>";

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

    xml::parseDeviceInfo(mediaServerRootDesc, dev);

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

TEST(XmlParseTest, GetStateVariablesFromDescription)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive>(serviceDesc.c_str());

    auto vars = xml::getStateVariablesFromDescription(doc);

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

TEST(XmlParseTest, ParseBrowseResponse)
{
    ContentDirectory::ActionResult result;
    auto didlLite = xml::parseBrowseResult(testxmls::browseResponse, result);

    EXPECT_EQ(10u, result.numberReturned);
    EXPECT_EQ(12u, result.totalMatches);
    EXPECT_EQ(2u, result.updateId);
    EXPECT_EQ(xml::decode("&lt;DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:arib=\"urn:schemas-arib-or-jp:elements-1-0/\" xmlns:dtcp=\"urn:schemas-dtcp-com:metadata-1-0/\" xmlns:pv=\"http://www.pv.com/pvns/\" xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\"&gt;&lt;item id=&quot;0$1$12$38502R2290700&quot; refID=&quot;0$1$8I2290700&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Momove&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;1&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2290700.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:playcount&gt;1&lt;/pv:playcount&gt;&lt;pv:modificationTime&gt;1133382780&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:lastPlayedTime&gt;2015-12-31T22:54:22&lt;/pv:lastPlayedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDEgLSBVbmRlcndvcmxkIC0gTW9tb3ZlLm1wMw==&lt;/pv:bookmark&gt;&lt;res duration=\"0:06:54\" size=\"10053760\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2290700.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2290700.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2291724&quot; refID=&quot;0$1$8I2291724&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Two Months Off&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;2&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2291724.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382778&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDIgLSBVbmRlcndvcmxkIC0gVHdvIE1vbnRocyBPZmYubXAz&lt;/pv:bookmark&gt;&lt;res duration=\"0:09:09\" size=\"13275264\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2291724.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291724.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2291212&quot; refID=&quot;0$1$8I2291212&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Twist&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;3&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2291212.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382776&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDMgLSBVbmRlcndvcmxkIC0gVHdpc3QubXAz&lt;/pv:bookmark&gt;&lt;res duration=\"0:06:24\" size=\"9330816\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2291212.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291212.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2291980&quot; refID=&quot;0$1$8I2291980&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Sola Sistim&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;4&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2291980.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382776&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDQgLSBVbmRlcndvcmxkIC0gU29sYSBTaXN0aW0ubXAz&lt;/pv:bookmark&gt;&lt;res duration=\"0:06:27\" size=\"9396352\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2291980.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291980.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2290956&quot; refID=&quot;0$1$8I2290956&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Little Speaker&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;5&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2290956.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382778&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDUgLSBVbmRlcndvcmxkIC0gTGl0dGxlIFNwZWFrZXIubXAz&lt;/pv:bookmark&gt;&lt;res duration=\"0:08:39\" size=\"12556416\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2290956.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2290956.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2292748&quot; refID=&quot;0$1$8I2292748&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Trim&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;6&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2292748.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382778&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDYgLSBVbmRlcndvcmxkIC0gVHJpbS5tcDM=&lt;/pv:bookmark&gt;&lt;res duration=\"0:03:24\" size=\"4999296\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2292748.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2292748.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2292492&quot; refID=&quot;0$1$8I2292492&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Ess Gee&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;7&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2292492.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382778&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDcgLSBVbmRlcndvcmxkIC0gRXNzIEdlZS5tcDM=&lt;/pv:bookmark&gt;&lt;res duration=\"0:02:22\" size=\"3516544\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2292492.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2292492.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2292236&quot; refID=&quot;0$1$8I2292236&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Dinosaur Adventure 3D&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;8&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2292236.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382774&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDggLSBVbmRlcndvcmxkIC0gRGlub3NhdXIgQWR2ZW50dXJlIDNELm1wMw==&lt;/pv:bookmark&gt;&lt;res duration=\"0:07:56\" size=\"11536512\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2292236.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2292236.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2293004&quot; refID=&quot;0$1$8I2293004&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Ballet Lane&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;9&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2293004.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:modificationTime&gt;1133382776&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMDkgLSBVbmRlcndvcmxkIC0gQmFsbGV0IExhbmUubXAz&lt;/pv:bookmark&gt;&lt;res duration=\"0:03:40\" size=\"5384320\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2293004.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2293004.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id=&quot;0$1$12$38502R2291468&quot; refID=&quot;0$1$8I2291468&quot; parentID=&quot;0$1$12$38502&quot; restricted=&quot;1&quot;&gt;&lt;dc:title&gt;Leutin&lt;/dc:title&gt;&lt;dc:date&gt;2002-01-01&lt;/dc:date&gt;&lt;upnp:genre&gt;Club&lt;/upnp:genre&gt;&lt;upnp:album&gt;A Hundred Days Off&lt;/upnp:album&gt;&lt;upnp:originalTrackNumber&gt;10&lt;/upnp:originalTrackNumber&gt;&lt;dc:creator&gt;Underworld&lt;/dc:creator&gt;&lt;upnp:albumArtURI dlna:profileID=\"JPEG_TN\" &gt;http://192.168.1.13:9000/disk/DLNA-PNJPEG_TN-OP01-CI1-FLAGS00d00000/defaa/A/O0$1$8I2291468.jpg?scale=org&lt;/upnp:albumArtURI&gt;&lt;upnp:artist&gt;Underworld&lt;/upnp:artist&gt;&lt;upnp:albumArtist&gt;Underworld&lt;/upnp:albumArtist&gt;&lt;pv:playcount&gt;1&lt;/pv:playcount&gt;&lt;pv:modificationTime&gt;1133382780&lt;/pv:modificationTime&gt;&lt;pv:addedTime&gt;1438685937&lt;/pv:addedTime&gt;&lt;pv:lastPlayedTime&gt;2016-03-28T15:38:02&lt;/pv:lastPlayedTime&gt;&lt;pv:numberOfThisDisc&gt;1&lt;/pv:numberOfThisDisc&gt;&lt;pv:extension&gt;mp3&lt;/pv:extension&gt;&lt;pv:bookmark&gt;uuid:55076f6e-6b79-1d65-a4eb-00089be34071,-L3NoYXJlL011bHRpbWVkaWEvbXVzaWMvVW5kZXJ3b3JsZCAtIEEgSHVuZHJlZCBEYXlzIE9mZiAoMjAwMikvMTAgLSBVbmRlcndvcmxkIC0gTGV1dGluLm1wMw==&lt;/pv:bookmark&gt;&lt;res duration=\"0:07:01\" size=\"10207360\" bitrate=\"24000\" protocolInfo=\"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=11;DLNA.ORG_FLAGS=01700000000000000000000000000000\" pv:timeseekinfo=\"http://192.168.1.13:9000/disk/O0$1$12$38502R2291468.seek\" &gt;http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291468.mp3&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;/DIDL-Lite&gt;"), didlLite);
}

}
}
