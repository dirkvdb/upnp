#include <gtest/gtest.h>
#include <pugixml.hpp>
#include <sstream>

#include "utils/log.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

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

    pugi::xml_document doc;
    EXPECT_TRUE(doc.load_buffer(gatewayRootDesc.c_str(), gatewayRootDesc.size()));

    doc.child("root").child("device").remove_child("friendlyName");

    std::stringstream xmlMod;
    doc.save(xmlMod);
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, MissingUdn)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    pugi::xml_document doc;
    EXPECT_TRUE(doc.load_buffer(gatewayRootDesc.c_str(), gatewayRootDesc.size()));

    doc.child("root").child("device").remove_child("UDN");

    std::stringstream xmlMod;
    doc.save(xmlMod);
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

TEST(XmlParseTest, MissingDeviceType)
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    pugi::xml_document doc;
    EXPECT_TRUE(doc.load_buffer(gatewayRootDesc.c_str(), gatewayRootDesc.size()));

    doc.child("root").child("device").remove_child("deviceType");

    std::stringstream xmlMod;
    doc.save(xmlMod);
    EXPECT_THROW(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

}
}
