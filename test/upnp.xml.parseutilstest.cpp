#include <catch.hpp>
#include <pugixml.hpp>
#include <sstream>

#include "utils/log.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"

namespace upnp
{
namespace test
{

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

TEST_CASE("XML Parse Gateway Device info", "[XML]")
{
    Device dev;
    dev.m_location = "http://192.168.1.1:5000/rootDesc.xml";

    xml::parseDeviceInfo(gatewayRootDesc, dev);

    CHECK(dev.m_udn == "uuid:A37351C5-8521-4c24-A43E-5C353B9982A9");
    CHECK(dev.m_friendlyName == "Compal Broadband Networks, Inc CH6643");
    CHECK(dev.m_baseURL.empty());
    CHECK(dev.m_type == DeviceType::InternetGateway);
    CHECK(dev.m_presURL.empty());
}

TEST_CASE("XML Parse Media server info", "[XML]")
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    xml::parseDeviceInfo(mediaServerRootDesc, dev);

    CHECK(dev.m_udn == "uuid:55076f6e-6b79-1d65-a4eb-00089be34071");
    CHECK(dev.m_friendlyName == "NAS");
    CHECK(dev.m_baseURL.empty());
    CHECK(dev.m_type == DeviceType::MediaServer);
    CHECK(dev.m_presURL == "http://192.168.1.13:9000/");

    CHECK(dev.m_services.size() == 2);

    auto& connMgrSvc = dev.m_services.at(ServiceType::ConnectionManager);
    CHECK(connMgrSvc.m_type                     == ServiceType::ConnectionManager);
    CHECK(connMgrSvc.m_id                       == "urn:upnp-org:serviceId:ConnectionManager");
    CHECK(connMgrSvc.m_controlURL               == "http://192.168.1.13:9000/dev0/srv0/control");
    CHECK(connMgrSvc.m_eventSubscriptionURL     == "http://192.168.1.13:9000/dev0/srv0/event");
    CHECK(connMgrSvc.m_scpdUrl                  == "http://192.168.1.13:9000/dev0/srv0.xml");

    auto& contDirMgrSvc = dev.m_services.at(ServiceType::ContentDirectory);
    CHECK(contDirMgrSvc.m_type                  == ServiceType::ContentDirectory);
    CHECK(contDirMgrSvc.m_id                    == "urn:upnp-org:serviceId:ContentDirectory");
    CHECK(contDirMgrSvc.m_controlURL            == "http://192.168.1.13:9000/dev0/srv1/control");
    CHECK(contDirMgrSvc.m_eventSubscriptionURL  == "http://192.168.1.13:9000/dev0/srv1/event");
    CHECK(contDirMgrSvc.m_scpdUrl               == "http://192.168.1.13:9000/dev0/srv1.xml");
}

TEST_CASE("XML Parse empty xml", "[XML]")
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    CHECK_THROWS_AS(xml::parseDeviceInfo("", dev), std::runtime_error);
}

TEST_CASE("Missing elements", "[XML]")
{
    Device dev;
    dev.m_location = "http://192.168.1.13:9000/desc.xml";

    pugi::xml_document doc;
    CHECK(doc.load_buffer(gatewayRootDesc.c_str(), gatewayRootDesc.size()));

    SECTION("No friendly name")
    {
        doc.child("root").child("device").remove_child("friendlyName");
    }

    SECTION("No UDN")
    {
        doc.child("root").child("device").remove_child("UDN");
    }

    SECTION("No DeviceType")
    {
        doc.child("root").child("device").remove_child("deviceType");
    }

    std::stringstream xmlMod;
    doc.save(xmlMod);

    CHECK_THROWS_AS(xml::parseDeviceInfo(xmlMod.str(), dev), std::runtime_error);
}

}
}
