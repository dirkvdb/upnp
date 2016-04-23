#include <gtest/gtest.h>
#include <sstream>

#include "utils/log.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml_print.hpp"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;
using namespace rapidxml_ns;

static const std::string gatewayRootDesc =
"<?xml version=\"1.0\"?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
"    <specVersion>"
"        <major>1</major>"
"        <minor>0</minor>"
"    </specVersion>"
"    <device>"
"        <deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>"
"        <friendlyName>Compal Broadband Networks, Inc CH6643</friendlyName>"
"        <manufacturer>Compal Broadband Networks, Inc</manufacturer>"
"        <manufacturerURL>http://www.icbn.com.tw</manufacturerURL>"
"        <modelDescription> CH6643</modelDescription>"
"        <modelName> CH6643</modelName>"
"        <modelNumber>CH6643</modelNumber>"
"        <modelURL>http://www.icbn.com.tw</modelURL>"
"        <serialNumber>601581307023619400000660</serialNumber>"
"        <UDN>uuid:A37351C5-8521-4c24-A43E-5C353B9982A9</UDN>"
"        <serviceList>"
"            <service>"
"                <serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>"
"                <serviceId>urn:upnp-org:serviceId:Layer3Forwarding1</serviceId>"
"                <controlURL>/ctl/L3F</controlURL>"
"                <eventSubURL>/evt/L3F</eventSubURL>"
"                <SCPDURL>/L3F.xml</SCPDURL>"
"            </service>"
"        </serviceList>"
"    </device>"
"</root>";

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

TEST(XmlParseTest, GatewayDeviceInfo)
{
    Device dev;
    dev.m_location = "http://192.168.1.1:5000/rootDesc.xml";

    xml::parseDeviceInfo(gatewayRootDesc, dev);

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

    auto xml = gatewayRootDesc;
    xml_document<> doc;
    doc.parse<parse_default>(&xml.front());
    doc.first_node_ref("root").first_node_ref("device").remove_node("friendlyName");

    std::stringstream xmlMod;
    xmlMod << *doc.first_node();
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, MissingUdn)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    auto xml = gatewayRootDesc;
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
    doc.parse<parse_non_destructive>(gatewayRootDesc.c_str());
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

}
}
