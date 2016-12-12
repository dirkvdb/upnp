#include "upnp/upnp.xml.parseutils.h"

#include <cassert>

#include "URI.h"
#include "utils/log.h"
#include "utils/format.h"
#include "utils/stringoperations.h"

#include "upnp/upnp.utils.h"
#include "upnp/upnp.device.h"
#include "upnp/upnpservicevariable.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

namespace upnp
{
namespace xml
{

using namespace utils;
using namespace rapidxml_ns;

static const char* s_valAtr         = "val";
static const char* s_xmlnsAtr = "xmlns:e";
static const char* s_ns = "urn:schemas-upnp-org:event-1-0";
static const char* s_propset = "e:propertyset";
static const char* s_prop = "e:property";
static const char* s_encAtr = "e:encodingStyle";
static const char* s_encVal = "http://schemas.xmlsoap.org/soap/encoding/";

namespace
{

bool findAndParseService(const xml_node<char>& node, const ServiceType::Type serviceType, Device& device)
{
    auto base = URI(device.baseURL.empty() ? device.location : device.baseURL);

    for (auto* serviceNode = node.first_node("service"); serviceNode != nullptr; serviceNode = serviceNode->next_sibling("service"))
    {
        Service service;
        service.type = serviceTypeUrnStringToService(requiredChildValue(*serviceNode, "serviceType"));
        if (service.type.type == serviceType)
        {
            service.id                    = requiredChildValue(*serviceNode, "serviceId");
            service.controlURL            = URI(base.getScheme(), base.getAuthority(), requiredChildValue(*serviceNode, "controlURL")).toString();
            service.eventSubscriptionURL  = URI(base.getScheme(), base.getAuthority(), requiredChildValue(*serviceNode, "eventSubURL")).toString();
            service.scpdUrl               = URI(base.getScheme(), base.getAuthority(), requiredChildValue(*serviceNode, "SCPDURL")).toString();

            device.services[serviceType] = service;
            return true;
        }
    }

    return false;
}

template <typename T>
T requiredChildValue(const xml_node<char>& node, const char* childName)
{
    std::string value = node.first_node_ref(childName).value_string();
    if (value.empty())
    {
        throw std::runtime_error(fmt::format("xml node ({}) child value not present: {}", node.name_string(), childName));
    }

    return stringops::toNumeric<T>(value);
}

template <typename T>
T optionalChildValue(const xml_node<char>& node, const char* childName, T defaultValue)
{
    auto* child = node.first_node(childName);
    if (!child)
    {
        return defaultValue;
    }

    return stringops::toNumeric<T>(child->value_string()); // TODO span
}

std::string requiredAttributeValue(xml_node<char>& elem, const char* name)
{
    auto* attr = elem.first_attribute(name);
    if (!attr)
    {
        throw std::runtime_error(fmt::format("Attribute not found in node '{}': {}", std::string(elem.name(), elem.name_size()), name));
    }

    assert(attr->name());
    return std::string(attr->value(), attr->value_size());
}

std::string optionalAttributeValue(xml_node<char>& elem, const char* name)
{
    auto* attr = elem.first_attribute(name);
    if (!attr || !attr->name())
    {
        return "";
    }

    return std::string(attr->value(), attr->value_size()); // TODO span
}

template <typename T>
T optionalAttributeValue(xml_node<char>& elem, const char* name, T defaultValue)
{
    auto* attr = elem.first_attribute(name);
    if (!attr || !attr->name())
    {
        return defaultValue;
    }

    return stringops::toNumeric<T>(std::string(attr->value(), attr->value_size())); // TODO span
}

std::vector<std::string> getActionsFromDescription(xml_document<char>& doc)
{
    std::vector<std::string> actions;

    auto& actionList = doc.first_node_ref().first_node_ref("actionList");
    for (auto* child = actionList.first_node(); child != nullptr; child = child->next_sibling())
    {
        actions.push_back(child->first_node_ref("name").value_string());
    }

    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(xml_document<char>& doc)
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
                    range->minimumValue    = requiredChildValue<uint32_t>(*rangeElem, "minimum");
                    range->maximumValue    = requiredChildValue<uint32_t>(*rangeElem, "maximum");
                    range->step            = optionalChildValue<uint32_t>(*rangeElem, "step", 0);

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

}

std::string encode(std::string_view data)
{
    std::string buffer;
    buffer.reserve(data.size() * 1.1f);

    for (auto character : data)
    {
        switch(character)
        {
            case '&':  buffer.append("&amp;", 5);       break;
            case '\"': buffer.append("&quot;", 6);      break;
            case '\'': buffer.append("&apos;", 6);      break;
            case '<':  buffer.append("&lt;", 4);        break;
            case '>':  buffer.append("&gt;", 4);        break;
            default:   buffer += character; break;
        }
    }

    return buffer;
}

std::string decode(std::string_view data)
{
    std::string buffer;
    buffer.reserve(data.size());

    for (size_t pos = 0; pos != data.size(); ++pos)
    {
        if (data[pos] == '&')
        {
            if (pos + 3 > data.size()) // minumum escape sequence has 3 more characters
            {
                buffer.append(&data[pos], 1);
                continue;
            }

            switch (data[pos+1])
            {
            case 'a':
            {
                switch (data[pos+2])
                {
                case 'm':
                {
                    if (pos + 4 > data.size()) // amp escape sequence has 4 more characters
                    {
                        buffer.append(&data[pos], 3);
                        pos += 2;
                        continue;
                    }

                    if (data[pos+3] == 'p' && data[pos+4] == ';')
                    {
                        buffer += '&';
                        pos += 4;
                    }
                    else
                    {
                        buffer.append(&data[pos], 3);
                        pos += 2;
                    }
                    break;
                }
                case 'p':
                {
                    if (pos + 5 > data.size()) // apos escape sequence has 5 more characters
                    {
                        buffer.append(&data[pos], 2);
                        pos += 1;
                        continue;
                    }

                    if (data[pos+3] == 'o' && data[pos+4] == 's' && data[pos+5] == ';')
                    {
                        buffer += '\'';
                        pos += 5;
                    }
                    else
                    {
                        buffer.append(&data[pos], 3);
                        pos += 2;
                    }
                    break;
                }
                }
                break;
            }
            case 'q':
            {
                if (pos + 5 > data.size()) // quot escape sequence has 5 more characters
                {
                    buffer.append(&data[pos], 2);
                    pos += 1;
                    continue;
                }

                if (data[pos+2] == 'u' && data[pos+3] == 'o' && data[pos+4] == 't' && data[pos+5] == ';')
                {
                    buffer += '\"';
                    pos += 5;
                }
                else
                {
                    buffer.append(&data[pos], 2);
                    pos += 1;
                }

                break;
            }
            case 'l':
            {
                if (data[pos+2] == 't' && data[pos+3] == ';')
                {
                    buffer += '<';
                    pos += 3;
                }
                else
                {
                    buffer.append(&data[pos], 2);
                    pos += 1;
                }

                break;
            }
            case 'g':
            {
                if (data[pos+2] == 't' && data[pos+3] == ';')
                {
                    buffer += '>';
                    pos += 3;
                }
                else
                {
                    buffer.append(&data[pos], 2);
                    pos += 1;
                }

                break;
            }
            default:
                buffer.append(&data[pos], 2);
                pos += 1;
                break;
            }
        }
        else
        {
            buffer.append(&data[pos], 1);
        }
    }

    return buffer;
}

void parseDeviceInfo(const std::string& xml, Device& device)
{
    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());

    auto& deviceNode = doc.first_node_ref("root").first_node_ref("device");
    device.udn            = requiredChildValue(deviceNode, "UDN");
    device.type           = deviceTypeFromString(requiredChildValue(deviceNode, "deviceType"));
    device.friendlyName   = requiredChildValue(deviceNode, "friendlyName");
    device.baseURL        = optionalChildValue(deviceNode, "URLBase");

    URI presUrl(optionalChildValue(deviceNode, "presentationURL"));
    if (!presUrl.empty())
    {
        if (presUrl.isRelative())
        {
            auto base = URI(device.baseURL.empty() ? device.location : device.baseURL);
            device.presURL = URI(base.getScheme(), base.getAuthority(), presUrl.toString()).toString();
        }
        else
        {
            device.presURL = presUrl.toString();
        }
    }

    auto& serviceListNode = deviceNode.first_node_ref("serviceList");

    if (device.type.type == DeviceType::MediaServer)
    {
        if (findAndParseService(serviceListNode, ServiceType::ContentDirectory, device))
        {
            // try to obtain the optional services
            findAndParseService(serviceListNode, ServiceType::AVTransport, device);
            findAndParseService(serviceListNode, ServiceType::ConnectionManager, device);
        }
    }
    else if (device.type.type == DeviceType::MediaRenderer)
    {
        if (findAndParseService(serviceListNode, ServiceType::RenderingControl, device) &&
            findAndParseService(serviceListNode, ServiceType::ConnectionManager, device))
        {
            // try to obtain the optional services
            findAndParseService(serviceListNode, ServiceType::AVTransport, device);
        }
    }
}

std::map<std::string, std::string> getEventValues(xml_document<char>& doc)
{
    std::map<std::string, std::string> values;
    auto& instanceNode = doc.first_node_ref().first_node_ref("InstanceID");
    for (auto* elem = instanceNode.first_node(); elem != nullptr; elem = elem->next_sibling())
    {
        values.emplace(elem->name_string(), requiredAttributeValue(*elem, "val"));
    }

    return values;
}

Resource parseResource(xml_node<char>& node, const std::string& url)
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
            else if (strncmp("bitrate", attr->name(), attr->name_size()) == 0)
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
        catch (const std::exception& e) { /* skip invalid resource */ log::warn(e.what()); }
    }

    return res;
}

Item parseContainer(xml_node<char>& containerElem)
{
    auto item = Item();
    item.setObjectId(requiredAttributeValue(containerElem, "id"));
    item.setParentId(requiredAttributeValue(containerElem, "parentID"));
    item.setRefId(optionalAttributeValue(containerElem, "refID"));
    item.setChildCount(optionalAttributeValue<uint32_t>(containerElem, "childCount", 0));
    item.setRestricted(optionalAttributeValue(containerElem, "restricted") != "0");

    for (auto* elem = containerElem.first_node(); elem != nullptr; elem = elem->next_sibling())
    {
        Property prop = propertyFromString(elem->name(), elem->name_size());
        if (prop == Property::Unknown)
        {
            item.addMetaData(elem->name_string(), decode(elem->value_view()));
            continue;
        }

        if (prop == Property::AlbumArt)
        {
            // multiple art uris can be present with different dlna profiles (size)
            try
            {
                auto& attrRef = elem->first_attribute_ref("dlna:profileID");
                item.setAlbumArt(dlna::profileIdFromString(attrRef.value(), attrRef.value_size()), decode(elem->value_view()));
            }
            catch (const std::exception&)
            {
                // no profile id present, add it as regular metadata
                item.addMetaData(prop, decode(elem->value_view()));
            }

            continue;
        }

        item.addMetaData(prop, decode(elem->value_view()));
    }

    // check required properties
    if (item.getTitle().empty())
    {
        throw std::runtime_error("No title found in item");
    }

    return item;
}

std::vector<Item> parseContainers(const std::string& xml)
{
    assert(!xml.empty() && "ParseContainers: Invalid document supplied");

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());
    auto& node = doc.first_node_ref();

    std::vector<Item> containers;
    for (auto* elem = node.first_node("container"); elem != nullptr; elem = elem->next_sibling("container"))
    {
        try
        {
            containers.push_back(parseContainer(*elem));
        }
        catch (const std::exception& e)
        {
            log::warn("Failed to parse container, skipping ({})", e.what());
        }
    }

    return containers;
}

Item parseItem(xml_node<char>& itemElem)
{
    auto item = Item();
    item.setObjectId(requiredAttributeValue(itemElem, "id"));
    item.setParentId(requiredAttributeValue(itemElem, "parentID"));
    item.setRefId(optionalAttributeValue(itemElem, "refID"));
    item.setRestricted(optionalAttributeValue(itemElem, "restricted") != "0");

    try
    {
        for (auto* elem = itemElem.first_node(); elem != nullptr; elem = elem->next_sibling())
        {
            try
            {
                auto prop = propertyFromString(elem->name(), elem->name_size());
                if (prop == Property::Res)
                {
                    item.addResource(parseResource(*elem, decode(elem->value_view())));
                }
                else if (prop == Property::AlbumArt)
                {
                    // multiple art uris can be present with different dlna profiles (size)
                    try
                    {
                        item.setAlbumArt(dlna::profileIdFromString(requiredAttributeValue(*elem, "dlna:profileID")), decode(elem->value_view()));
                    }
                    catch (std::exception&)
                    {
                        // no profile id present, add it as regular metadata
                        item.addMetaData(prop, decode(elem->value_view()));
                    }
                }
                else if (prop != Property::Unknown)
                {
                    item.addMetaData(prop, decode(elem->value_view()));
                }
                else
                {
                    item.addMetaData(elem->name_string(), decode(elem->value_view()));
                }
            }
            catch (const std::exception& e) { /* try to parse the rest */ log::warn("Failed to parse upnp item: {}", e.what()); }
        }
    }
    catch (const std::exception& e)
    {
        log::warn("Failed to parse item");
    }

    return item;
}

Item parseItemDocument(const std::string& xml)
{
    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());

    auto& elem = doc.first_node_ref();
    auto& itemElem = elem.first_node_ref();
    return parseItem(itemElem);
}

std::vector<Item> parseItems(const std::string& xml)
{
    assert(!xml.empty() && "ParseItems: Invalid document supplied");

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());
    auto& node = doc.first_node_ref();

    std::vector<Item> items;
    for (auto* elem = node.first_node("item"); elem != nullptr; elem = elem->next_sibling("item"))
    {
        try
        {
            items.push_back(parseItem(*elem));
        }
        catch (const std::exception& e)
        {
            log::error("Failed to parse item, skipping ({})", e.what());
        }
    }

    return items;
}

Item parseMetaData(const std::string& meta)
{
    assert(!meta.empty() && "ParseMetaData: Invalid document supplied");

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(meta.c_str());
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
        catch (const std::exception&) {}
    }

    log::warn("No metadata could be retrieved");
    return Item();
}

std::string parseBrowseResult(const std::string& response, ContentDirectory::ActionResult& result)
{
    assert(!response.empty() && "ParseBrowseResult: Invalid document supplied");

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(response.c_str());

    auto& browseResultNode = doc.first_node_ref().first_node_ref().first_node_ref();
    if (strncmp("BrowseResponse", browseResultNode.local_name(), browseResultNode.local_name_size()) != 0)
    {
        throw std::runtime_error("Failed to find BrowseResponse node in browse result");
    }

    std::string browseResult;
    for (auto* child = browseResultNode.first_node(); child != nullptr; child = child->next_sibling())
    {
        if (strncmp("Result", child->name(), child->name_size()) == 0)
        {
            browseResult = decode(child->value_view());
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
        throw std::runtime_error("Failed to obtain browse result");
    }

    return browseResult;
}

void parseEvent(const std::string& data, std::function<void(const std::string& varable, const std::map<std::string, std::string>&)> cb)
{
    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(data.c_str());
    auto& propertySet = doc.first_node_ref();
    for (auto* property = propertySet.first_node(); property != nullptr; property = property->next_sibling())
    {
        for (auto* var = property->first_node(); var != nullptr; var = var->next_sibling())
        {
            auto changedVar = var->name_string();

            auto decoded = decode(var->value_string()); // TODO avoid copy
            xml_document<char> changeDoc;
            changeDoc.parse<parse_non_destructive | parse_trim_whitespace>(decoded.c_str());
            auto& instanceIDNode = changeDoc.first_node_ref().first_node_ref("InstanceID");

            std::map<std::string, std::string> vars;
            for (auto* elem = instanceIDNode.first_node(); elem != nullptr; elem = elem->next_sibling())
            {
                vars.emplace(elem->name_string(), requiredAttributeValue(*elem, "val"));
            }

            cb(changedVar, vars);
        }
    }
}

std::vector<StateVariable> parseServiceDescription(const std::string& contents, std::function<void(const std::string& action)> actionCb)
{
    xml_document<char> doc;
    doc.parse<parse_non_destructive>(contents.c_str());

    for (auto& action : xml::getActionsFromDescription(doc))
    {
        actionCb(action);
    }

    return xml::getStateVariablesFromDescription(doc);
}

std::string createNotificationXml(const std::vector<std::pair<std::string, std::string>>& vars)
{
    xml_document<> doc;

    auto* propset = doc.allocate_node(node_element, s_propset);
    propset->append_attribute(doc.allocate_attribute(s_xmlnsAtr, s_ns));
    propset->append_attribute(doc.allocate_attribute(s_encAtr, s_encVal));

    for (auto& var : vars)
    {
        auto* prop = doc.allocate_node(node_element, s_prop);
        prop->append_node(doc.allocate_node(node_element, var.first.c_str(), var.second.c_str()));
        propset->append_node(prop);
    }

    doc.append_node(propset);

    return xml::toString(doc);
}

std::string optionalChildValue(const xml_node<char>& node, const char* child)
{
    std::string result;

    auto* childNode = node.first_node(child);
    if (childNode && childNode->value())
    {
        result.assign(std::string(childNode->value(), childNode->value_size()));
    }

    return result;
}

std::string requiredChildValue(const xml_node<char>& node, const char* childName)
{
    std::string value = node.first_node_ref(childName).value_string();
    if (value.empty())
    {
        throw std::runtime_error(fmt::format("xml node ({}) child value not present: {}", node.name_string(), childName));
    }

    return value;
}

rapidxml_ns::xml_node<char>* serviceVariableToElement(rapidxml_ns::xml_document<char>& doc, const ServiceVariable& var)
{
    auto* varNode = doc.allocate_node(node_element, var.getName().c_str());
    varNode->append_attribute(doc.allocate_attribute(s_valAtr, var.getValue().c_str()));

    auto& attr = var.getAttribute();
    if (!attr.first.empty())
    {
        varNode->append_attribute(doc.allocate_attribute(attr.first.c_str(), attr.second.c_str()));
    }

    return varNode;
}

namespace
{

xml_node<char>* createDidlForDocument(xml_document<char>& doc)
{
    auto* didl = doc.allocate_node(node_element, "DIDL-Lite");
    didl->append_attribute(doc.allocate_attribute("xmlns", "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"));
    didl->append_attribute(doc.allocate_attribute("xmlns:dc", "http://purl.org/dc/elements/1.1/"));
    didl->append_attribute(doc.allocate_attribute("xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/"));
    didl->append_attribute(doc.allocate_attribute("xmlns:dlna", "urn:schemas-dlna-org:metadata-1-0/"));

    return didl;
}

void addItemToDidl(xml_document<char>& doc, xml_node<char>& didl, const Item& item)
{
    bool isContainer = item.isContainer();
    auto itemElem = doc.allocate_node(node_element, isContainer ? "container" : "item");

    itemElem->append_attribute(doc.allocate_attribute("id", item.getObjectId().c_str()));
    itemElem->append_attribute(doc.allocate_attribute("parentID", item.getParentId().c_str()));
    itemElem->append_attribute(doc.allocate_attribute("restricted", item.restricted() ? "1" : "0"));

    if (isContainer)
    {
        itemElem->append_attribute(doc.allocate_attribute("childCount", doc.allocate_string(std::to_string(item.getChildCount()).c_str())));
    }

    for (auto& meta : item.getMetaData())
    {
        try
        {
            auto elem = doc.allocate_node(node_element, toString(meta.first));
            auto node = doc.allocate_node(node_element, meta.second.c_str());
            elem->append_node(node);
            itemElem->append_node(elem);
        }
        catch (const std::exception&) { /* Unknown metadata */ }
    }

    for (auto& uri : item.getAlbumArtUris())
    {
        try
        {
            auto elem = doc.allocate_node(node_element, toString(Property::AlbumArt));
            auto node = doc.allocate_node(node_element, uri.second.c_str());
            elem->append_attribute(doc.allocate_attribute("dlna:profileID", dlna::toString(uri.first)));
            elem->append_node(node);
            itemElem->append_node(elem);
        }
        catch (const std::exception&) { /* Unknown profileId */ }
    }

    for (auto& res : item.getResources())
    {
        try
        {
            auto elem = doc.allocate_node(node_element, "res");
            auto node = doc.allocate_node(node_element, res.getUrl().c_str());
            elem->append_attribute(doc.allocate_attribute("protocolInfo", doc.allocate_string(res.getProtocolInfo().toString().c_str())));

            auto size = res.getSize();
            if (size > 0) { elem->append_attribute(doc.allocate_attribute("size", doc.allocate_string(std::to_string(size).c_str()))); }

            if (item.getClass() == upnp::Class::Audio)
            {
                auto duration = res.getDuration();
                if (duration.count() > 0) { elem->append_attribute(doc.allocate_attribute("duration", doc.allocate_string(durationToString(duration).c_str()))); }

                auto bitrate = res.getBitRate();
                if (bitrate > 0) { elem->append_attribute(doc.allocate_attribute("bitrate", doc.allocate_string(std::to_string(bitrate).c_str()))); }

                auto sampleRate = res.getSampleRate();
                if (sampleRate > 0) { elem->append_attribute(doc.allocate_attribute("samplefrequency", doc.allocate_string(std::to_string(sampleRate).c_str()))); }

                auto nrChannels = res.getNrAudioChannels();
                if (nrChannels > 0) { elem->append_attribute(doc.allocate_attribute("nrAudioChannels", doc.allocate_string(std::to_string(nrChannels).c_str()))); }

                auto bitsPerSample = res.getBitsPerSample();
                if (bitsPerSample > 0) { elem->append_attribute(doc.allocate_attribute("bitsPerSample", doc.allocate_string(std::to_string(bitsPerSample).c_str()))); }

                elem->append_node(node);
                itemElem->append_node(elem);
            }
        }
        catch (const std::exception&) {}
    }

    didl.append_node(itemElem);
}

}

std::string getItemDocument(const Item& item)
{
    xml_document<char> doc;
    auto* didl = createDidlForDocument(doc);
    addItemToDidl(doc, *didl, item);
    doc.append_node(didl);
    return toString(doc);
}

std::string getItemsDocument(const std::vector<Item>& items)
{
    xml_document<char> doc;
    auto* didl = createDidlForDocument(doc);

    for (auto& item : items)
    {
        addItemToDidl(doc, *didl, item);
    }

    doc.append_node(didl);
    return toString(doc);
}

std::string toString(xml_document<char>& doc)
{
    std::string result("<?xml version=\"1.0\"?>");
    rapidxml_ns::print(std::back_inserter(result), doc, print_no_indenting);
    return result;
}

std::string toString(xml_node<char>& node)
{
    std::string result;
    rapidxml_ns::print(std::back_inserter(result), node, print_no_indenting);
    return result;
}

}
}
