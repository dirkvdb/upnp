#include "upnp/upnp.xml.parseutils.h"
#include "upnp/upnpdevice.h"

#include <cassert>
#include <pugixml.hpp>

#include "URI.h"
#include "utils/log.h"
#include "utils/format.h"

namespace upnp
{
namespace xml
{

using namespace utils;

namespace
{

std::string requiredChildValue(const pugi::xml_node& node, const char* childName)
{
    std::string value = node.child_value(childName);
    if (value.empty())
    {
        throw std::runtime_error(fmt::format("xml node ({}) child value not present: {}", node.name(), childName));
    }

    return value;
}

bool findAndParseService(const pugi::xml_node& node, const ServiceType serviceType, Device& device)
{
    auto base = URI(device.m_baseURL.empty() ? device.m_location : device.m_baseURL);

    assert(!node.empty());

    for (auto& serviceNode : node.children("service"))
    {
        Service service;
        service.m_type = serviceTypeUrnStringToService(requiredChildValue(serviceNode, "serviceType"));
        if (service.m_type == serviceType)
        {
            service.m_id                    = requiredChildValue(serviceNode, "serviceId");
            service.m_controlURL            = URI(base, requiredChildValue(serviceNode, "controlURL")).toString();
            service.m_eventSubscriptionURL  = URI(base, requiredChildValue(serviceNode, "eventSubURL")).toString();
            service.m_scpdUrl               = URI(base, requiredChildValue(serviceNode, "SCPDURL")).toString();

            device.m_services[serviceType] = service;
            return true;
        }
    }

    return false;
}

}

void parseDeviceInfo(const std::string& xml, Device& device)
{
    pugi::xml_document doc;
    auto result = doc.load_buffer(xml.c_str(), xml.size());

    if (!result)
    {
        throw std::runtime_error("Failed to load device info xml");
    }

    auto deviceNode = doc.child("root").child("device");
    device.m_udn            = requiredChildValue(deviceNode, "UDN");
    device.m_type           = Device::stringToDeviceType(requiredChildValue(deviceNode, "deviceType"));
    device.m_friendlyName   = requiredChildValue(deviceNode, "friendlyName");
    device.m_baseURL        = deviceNode.child_value("URLBase");

    URI presUrl(deviceNode.child_value("presentationURL"));
    if (!presUrl.empty())
    {
        if (presUrl.isRelative())
        {
            device.m_presURL = URI(URI(device.m_baseURL.empty() ? device.m_location : device.m_baseURL, presUrl.toString())).toString();
        }
        else
        {
            device.m_presURL = presUrl.toString();
        }
    }

    auto serviceListNode = deviceNode.child("serviceList");

    if (device.m_type == DeviceType::MediaServer)
    {
        if (findAndParseService(serviceListNode, ServiceType::ContentDirectory, device))
        {
            // try to obtain the optional services
            findAndParseService(serviceListNode, ServiceType::AVTransport, device);
            findAndParseService(serviceListNode, ServiceType::ConnectionManager, device);
        }
    }
    else if (device.m_type == DeviceType::MediaRenderer)
    {
        if (findAndParseService(serviceListNode, ServiceType::RenderingControl, device) &&
            findAndParseService(serviceListNode, ServiceType::ConnectionManager, device))
        {
            // try to obtain the optional services
            findAndParseService(serviceListNode, ServiceType::AVTransport, device);
        }
    }
}

}
}
