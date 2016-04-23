#include "upnp/upnp.xml.parseutils.h"
#include "upnp/upnpdevice.h"

#include <cassert>

#include "URI.h"
#include "utils/log.h"
#include "utils/format.h"
#include "utils/stringoperations.h"

#include "upnp/upnputils.h"

namespace upnp
{
namespace xml
{

using namespace utils;
using namespace rapidxml_ns;

namespace
{

std::string requiredChildValue(const xml_node<>& node, const char* childName)
{
    std::string value = node.first_node_ref(childName).value_string();
    if (value.empty())
    {
        throw std::runtime_error(fmt::format("xml node ({}) child value not present: {}", node.name_string(), childName));
    }

    return value;
}

std::string optionalChildValue(const xml_node<>& node, const char* childName)
{
    auto* child = node.first_node(childName);
    if (child)
    {
        return child->value_string();
    }

    return std::string();
}

std::string requiredAttributeValue(xml_node<>& elem, const char* name)
{
    auto* attr = elem.first_attribute(name);
    if (!attr)
    {
        throw std::runtime_error(fmt::format("Attribute not found in node '{}': {}", std::string(elem.name(), elem.name_size()), name));
    }

    assert(attr->name());
    return std::string(attr->value(), attr->value_size());
}

template <typename T>
T optionalAttributeValue(xml_node<>& elem, const char* name, T defaultValue)
{
    auto* attr = elem.first_attribute(name);
    if (!attr || !attr->name())
    {
        return defaultValue;
    }

    return stringops::toNumeric<T>(std::string(attr->name(), attr->name_size())); // TODO span
}

bool findAndParseService(const xml_node<>& node, const ServiceType serviceType, Device& device)
{
    auto base = URI(device.m_baseURL.empty() ? device.m_location : device.m_baseURL);

    for (auto* serviceNode = node.first_node("service"); serviceNode != nullptr; serviceNode = serviceNode->next_sibling("service"))
    {
        Service service;
        service.m_type = serviceTypeUrnStringToService(requiredChildValue(*serviceNode, "serviceType"));
        if (service.m_type == serviceType)
        {
            service.m_id                    = requiredChildValue(*serviceNode, "serviceId");
            service.m_controlURL            = URI(base, requiredChildValue(*serviceNode, "controlURL")).toString();
            service.m_eventSubscriptionURL  = URI(base, requiredChildValue(*serviceNode, "eventSubURL")).toString();
            service.m_scpdUrl               = URI(base, requiredChildValue(*serviceNode, "SCPDURL")).toString();

            device.m_services[serviceType] = service;
            return true;
        }
    }

    return false;
}

void addPropertyToItem(const std::string& propertyName, const std::string& propertyValue, Item& item)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        item.addMetaData(prop, propertyValue);
    }
    else
    {
        log::warn("Unknown property: {}", propertyName);
    }
}

}

void parseDeviceInfo(const std::string& xml, Device& device)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive>(xml.c_str());

    auto& deviceNode = doc.first_node_ref("root").first_node_ref("device");
    device.m_udn            = requiredChildValue(deviceNode, "UDN");
    device.m_type           = Device::stringToDeviceType(requiredChildValue(deviceNode, "deviceType"));
    device.m_friendlyName   = requiredChildValue(deviceNode, "friendlyName");
    device.m_baseURL        = optionalChildValue(deviceNode, "URLBase");

    URI presUrl(optionalChildValue(deviceNode, "presentationURL"));
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

    auto& serviceListNode = deviceNode.first_node_ref("serviceList");

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

std::vector<std::string> getActionsFromDescription(xml_document<>& doc)
{
    std::vector<std::string> actions;

    auto& actionList = doc.first_node_ref().first_node_ref("actionList");
    for (auto* child = actionList.first_node(); child != nullptr; child = child->next_sibling())
    {
        actions.push_back(child->first_node_ref("name").value_string());
    }

    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(xml_document<>& doc)
{
    std::vector<StateVariable> variables;

    auto& stateVariableList = doc.first_node_ref().first_node_ref("serviceStateTable");
    for (auto* elem = stateVariableList.first_node("stateVariable"); elem != nullptr; elem = elem->next_sibling("stateVariable"))
    {
        try
        {
            StateVariable var;
            var.sendsEvents = requiredAttributeValue(*elem, "sendEvents") == "yes";
            var.name        = elem->first_node_ref("name").value_string();
            var.dataType    = elem->first_node_ref("dataType").value_string();

            auto* rangeElem = elem->first_node("allowedValueRange");
            if (rangeElem)
            {
                try
                {
                    auto range             = std::make_unique<StateVariable::ValueRange>();
                    range->minimumValue    = stringops::toNumeric<uint32_t>(rangeElem->first_node_ref("minimum").value_string());
                    range->maximumValue    = stringops::toNumeric<uint32_t>(rangeElem->first_node_ref("maximum").value_string());
                    range->step            = stringops::toNumeric<uint32_t>(rangeElem->first_node_ref("step").value_string());
                    var.valueRange = std::move(range);
                }
                catch(std::exception& e)
                {
                    log::warn("Failed to parse value range: {}", e.what());
                }
            }

            variables.emplace_back(std::move(var));
        }
        catch(std::exception& e)
        {
            log::warn("Failed to parse state variable, skipping: {}", e.what());
        }
    }

    return variables;
}

std::map<std::string, std::string> getEventValues(xml_document<>& doc)
{
    std::map<std::string, std::string> values;
    auto& instanceNode = doc.first_node_ref().first_node_ref("InstanceID");
    for (auto* elem = instanceNode.first_node(); elem != nullptr; elem = elem->next_sibling())
    {
        values.emplace(elem->name_string(), requiredAttributeValue(*elem, "val"));
    }
    
    return values;
}

Resource parseResource(xml_node<>& node, const std::string& url)
{
    Resource res;
    res.setUrl(url);

    for (auto* attr = node.first_attribute(); attr != nullptr; attr = attr->next_attribute())
    {
        try
        {
            if (strncmp("protocolInfo", attr->name(), attr->name_size()) == 0)
            {
                res.setProtocolInfo(ProtocolInfo(attr->value_string()));
            }
            else if (strncmp("size", attr->name(), attr->name_size()) == 0)
            {
                res.setSize(xml::optionalStringToUnsignedNumeric<uint64_t>(attr->value_string()));
            }
            else if (strncmp("duration", attr->name(), attr->name_size()) == 0)
            {
                res.setDuration(durationFromString(attr->value_string()));
            }
            else if (strncmp("nrAudioChannels", attr->name(), attr->name_size()) == 0)
            {
                res.setNrAudioChannels(xml::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("bitRate", attr->name(), attr->name_size()) == 0)
            {
                res.setBitRate(xml::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("sampleFrequency", attr->name(), attr->name_size()) == 0)
            {
                res.setSampleRate(xml::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("bitsPerSample", attr->name(), attr->name_size()) == 0)
            {
                res.setBitsPerSample(xml::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else
            {
                res.addMetaData(node.name_string(), attr->value_string());
            }
        }
        catch (std::exception& e) { /* skip invalid resource */ log::warn(e.what()); }
    }

    return res;
}

Item parseContainer(xml_node<>& containerElem)
{
    auto item = Item();
    item.setObjectId(requiredAttributeValue(containerElem, "id"));
    item.setParentId(requiredAttributeValue(containerElem, "parentID"));
    item.setChildCount(optionalAttributeValue<uint32_t>(containerElem, "childCount", 0));

    for (auto* elem = containerElem.first_node(); elem != nullptr; elem = elem->next_sibling())
    {
        Property prop = propertyFromString(elem->name_string());
        if (prop == Property::Unknown)
        {
            log::warn("Unknown property {}", elem->name_string());
            continue;
        }

        item.addMetaData(prop, elem->value_string());
    }

    // check required properties
    if (item.getTitle().empty())
    {
        throw Exception("No title found in item");
    }

    return item;
}

std::vector<Item> parseContainers(const std::string& xml)
{
    assert(!xml.empty() && "ParseContainers: Invalid document supplied");

    xml_document<> doc;
    doc.parse<parse_non_destructive>(xml.c_str());
    auto& node = doc.first_node_ref();

    std::vector<Item> containers;
    for (auto* elem = node.first_node("container"); elem != nullptr; elem = elem->next_sibling("container"))
    {
        try
        {
            containers.push_back(parseContainer(*elem));
        }
        catch (std::exception& e)
        {
            log::warn("Failed to parse container, skipping ({})", e.what());
        }
    }

    return containers;
}

Item parseItem(xml_node<>& itemElem)
{
    auto item = Item();
    item.setObjectId(requiredAttributeValue(itemElem, "id"));
    item.setParentId(requiredAttributeValue(itemElem, "parentID"));

    try
    {
        for (auto* elem = itemElem.first_node(); elem != nullptr; elem = elem->next_sibling())
        {
            try
            {
                if (strncmp("res", elem->name(), elem->name_size()) == 0)
                {
                    item.addResource(parseResource(*elem, elem->value_string()));
                }
                else if (strncmp("upnp:albumArtURI", elem->name(), elem->name_size()) == 0)
                {
                    // multiple art uris can be present with different dlna profiles (size)
                    try
                    {
                        item.setAlbumArt(dlna::profileIdFromString(requiredAttributeValue(*elem, "dlna:profileID")), elem->value_string());
                    }
                    catch (std::exception&)
                    {
                        // no profile id present, add it as regular metadata
                        addPropertyToItem(elem->name_string(), elem->value_string(), item);
                    }
                }
                else
                {
                    addPropertyToItem(elem->name_string(), elem->value_string(), item);
                }
            }
            catch (std::exception& e) { /* try to parse the rest */ log::warn("Failed to parse upnp item: {}", e.what()); }
        }
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item");
    }

    return item;
}

std::vector<Item> parseItems(const std::string& xml)
{
    assert(!xml.empty() && "ParseItems: Invalid document supplied");

    xml_document<> doc;
    doc.parse<parse_non_destructive>(xml.c_str());
    auto& node = doc.first_node_ref();

    std::vector<Item> items;
    for (auto* elem = node.first_node("item"); elem != nullptr; elem = elem->next_sibling("item"))
    {
        try
        {
            items.push_back(parseItem(*elem));
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse item, skipping ({})", e.what());
        }
    }

    return items;
}

Item parseMetaData(const std::string& meta)
{
    assert(!meta.empty() && "ParseMetaData: Invalid document supplied");

    xml_document<> doc;
    doc.parse<parse_non_destructive>(meta.c_str());
    auto& rootNode = doc.first_node_ref();

    auto* node = rootNode.first_node();
    if (node)
    {
        try
        {
            if (strncmp("container", node->name(), node->name_size()) == 0)
            {

                return parseContainer(*node);
            }
            else if (strncmp("item", node->name(), node->name_size()) == 0)
            {
                return parseItem(*node);
            }
        }
        catch (std::exception&) {}
    }

    log::warn("No metadata could be retrieved");
    return Item();
}

std::string parseBrowseResult(const std::string& response, ContentDirectory::ActionResult& result)
{
    assert(!response.empty() && "ParseBrowseResult: Invalid document supplied");

    xml_document<> doc;
    doc.parse<parse_non_destructive>(response.c_str());
    auto& browseResultNode = doc.first_node_ref();

    std::string browseResult;
    for (auto* child = browseResultNode.first_node(); child != nullptr; child = child->next_sibling())
    {
        if (strncmp("Result", child->name(), child->name_size()) == 0)
        {
            browseResult.assign(child->value(), child->value_size());
        }
        else if (strncmp("NumberReturned", child->name(), child->name_size()) == 0)
        {
            result.numberReturned = stringops::toNumeric<uint32_t>(child->value_string());
        }
        else if (strncmp("TotalMatches", child->name(), child->name_size()) == 0)
        {
            result.totalMatches = stringops::toNumeric<uint32_t>(child->value_string());
        }
        else if (strncmp("UpdateID", child->name(), child->name_size()) == 0)
        {
            result.updateId = stringops::toNumeric<uint32_t>(child->value_string());
        }
    }

    if (browseResult.empty())
    {
        throw Exception("Failed to obtain browse result");
    }

    return browseResult;
}


Item parseItemDocument(const std::string& xml)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive>(xml.c_str());

    auto& elem = doc.first_node_ref();
    auto& itemElem = elem.first_node_ref();
    return parseItem(itemElem);
}

}
}
