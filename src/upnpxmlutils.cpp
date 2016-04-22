//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "upnp/upnpxmlutils.h"
#include "upnp/upnpitem.h"
#include "upnp/upnpservicevariable.h"
#include "upnp/upnputils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

#include <cstring>
#include <stdexcept>

namespace upnp
{
namespace xml
{
namespace utils
{

using namespace ::utils;
using namespace rapidxml_ns;

namespace
{

std::string requiredAttributeValue(xml_node<>& elem, const char* name)
{
    auto* attr = elem.first_attribute(name);
    if (!attr)
    {
        throw std::runtime_error(fmt::format("Attribute not found in node '{}': {}", std::string(elem.name(), elem.name_size()), name));
    }

    assert(attr->name());
    return std::string(attr->name(), attr->name_size());
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

Element createDidlForDocument(Document& doc)
{
    auto didl = doc.createElement("DIDL-Lite");
    didl.addAttribute("xmlns", "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/");
    didl.addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    didl.addAttribute("xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");
    didl.addAttribute("xmlns:dlna", "urn:schemas-dlna-org:metadata-1-0/");

    return didl;
}

void addItemToDidl(Document& doc, Element& didl, const Item& item)
{
    bool isContainer = item.isContainer();
    auto itemElem = doc.createElement(isContainer ? "container" : "item");

    itemElem.addAttribute("id", item.getObjectId());
    itemElem.addAttribute("parentID", item.getParentId());
    itemElem.addAttribute("restricted", item.restricted() ? "1" : "0");

    if (isContainer)
    {
        itemElem.addAttribute("childCount", std::to_string(item.getChildCount()));
    }

    for (auto& meta : item.getMetaData())
    {
        try
        {
            auto elem = doc.createElement(toString(meta.first));
            auto node = doc.createNode(meta.second);
            elem.appendChild(node);
            itemElem.appendChild(elem);
        }
        catch (std::exception&) { /* Unknown metadata */ }
    }

    for (auto& uri : item.getAlbumArtUris())
    {
        try
        {
            auto elem = doc.createElement(toString(Property::AlbumArt));
            auto node = doc.createNode(uri.second);
            elem.addAttribute("dlna:profileID", dlna::toString(uri.first));
            elem.appendChild(node);
            itemElem.appendChild(elem);
        }
        catch (std::exception&) { /* Unknown profileId */ }
    }

    for (auto& res : item.getResources())
    {
        try
        {
            auto elem = doc.createElement("res");
            auto node = doc.createNode(res.getUrl());
            elem.addAttribute("protocolInfo", res.getProtocolInfo().toString());

            auto size = res.getSize();
            if (size > 0) { elem.addAttribute("size", std::to_string(size)); }

            if (item.getClass() == upnp::Class::Audio)
            {
                auto duration = res.getDuration();
                if (duration > 0) { elem.addAttribute("duration", durationToString(duration)); }

                auto bitrate = res.getBitRate();
                if (bitrate > 0) { elem.addAttribute("bitrate", std::to_string(bitrate)); }

                auto sampleRate = res.getSampleRate();
                if (sampleRate > 0) { elem.addAttribute("samplefrequency", std::to_string(sampleRate)); }

                auto nrChannels = res.getNrAudioChannels();
                if (nrChannels > 0) { elem.addAttribute("nrAudioChannels", std::to_string(nrChannels)); }

                auto bitsPerSample = res.getBitsPerSample();
                if (bitsPerSample > 0) { elem.addAttribute("bitsPerSample", std::to_string(bitsPerSample)); }

                elem.appendChild(node);
                itemElem.appendChild(elem);
            }
        }
        catch (std::exception&) {}
    }

    didl.appendChild(itemElem);
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

//<Event xmlns="urn:schemas-upnp-org:metadata-1-0/AVT/">
//  <InstanceID val="0">
//    <TransportState val="PLAYING"/>
//    <CurrentTrackURI val="http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2598668.mp3"/>
//    <CurrentTransportActions val="Pause,Stop,Next,Previous"/>
//  </InstanceID>
//</Event>

std::map<std::string, std::string> getEventValues(xml_document<>& doc)
{
    std::map<std::string, std::string> values;
    for (auto* elem = doc.first_node_ref().first_node("InstanceID"); elem != nullptr; elem = elem->next_sibling("InstanceID"))
    {
        values.emplace(elem->name_string(), requiredAttributeValue(*elem, "val"));
    }

    return values;
}

Document getItemDocument(const Item& item)
{
    Document doc;
    auto didl = createDidlForDocument(doc);
    addItemToDidl(doc, didl, item);
    doc.appendChild(didl);
    return doc;
}

Document getItemsDocument(const std::vector<Item>& items)
{
    Document doc;
    auto didl = createDidlForDocument(doc);

    for (auto& item : items)
    {
        addItemToDidl(doc, didl, item);
    }

    doc.appendChild(didl);
    return doc;
}

Element createServiceVariablesElement(Document& doc, uint32_t instanceId, const std::vector<ServiceVariable>& vars)
{
    auto instance = doc.createElement("InstanceID");
    instance.addAttribute("val", std::to_string(instanceId));

    for (auto& var : vars)
    {
        auto elem = serviceVariableToElement(doc, var);
        instance.appendChild(elem);
    }

    return instance;
}

Element serviceVariableToElement(Document& doc, const ServiceVariable& var)
{
    auto varElem = doc.createElement(var.getName());
    varElem.addAttribute("val", var.getValue());

    auto attr = var.getAttribute();
    if (!attr.first.empty())
    {
        varElem.addAttribute(attr.first, attr.second);
    }

    return varElem;
}

Resource parseResource(xml::NamedNodeMap& nodeMap, const std::string& url)
{
    Resource res;
    res.setUrl(url);

    for (auto& node : nodeMap)
    {
        try
        {
            std::string key = node.getName();
            std::string value = node.getValue();

            if (key == "protocolInfo")
            {
                res.setProtocolInfo(ProtocolInfo(value));
            }
            else if (key == "size")
            {
                res.setSize(optionalStringToUnsignedNumeric<uint64_t>(value));
            }
            else if (key == "duration")
            {
                res.setDuration(durationFromString(value));
            }
            else if (key == "nrAudioChannels")
            {
                res.setNrAudioChannels(optionalStringToUnsignedNumeric<uint32_t>(value));
            }
            else if (key == "bitRate")
            {
                res.setBitRate(optionalStringToUnsignedNumeric<uint32_t>(value));
            }
            else if (key == "sampleFrequency")
            {
                res.setSampleRate(optionalStringToUnsignedNumeric<uint32_t>(value));
            }
            else if (key == "bitsPerSample")
            {
                res.setBitsPerSample(optionalStringToUnsignedNumeric<uint32_t>(value));
            }
            else
            {
                res.addMetaData(key, value);
            }
        }
        catch (std::exception& e) { /* skip invalid resource */ log::warn(e.what()); }
    }

    return res;
}

static void addPropertyToItem(const std::string& propertyName, const std::string& propertyValue, Item& item)
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

Item parseItem(xml::Element& itemElem)
{
    auto item = Item();
    item.setObjectId(itemElem.getAttribute("id"));
    item.setParentId(itemElem.getAttribute("parentID"));

    try
    {
        for (xml::Element elem : itemElem.getChildNodes())
        {
            try
            {
                std::string key     = elem.getName();
                std::string value   = elem.getValue();

                if ("res" == key)
                {
                    auto nodeMap = elem.getAttributes();
                    item.addResource(parseResource(nodeMap, value));
                }
                else if ("upnp:albumArtURI" == key)
                {
                    // multiple art uris can be present with different dlna profiles (size)
                    try
                    {
                        item.setAlbumArt(dlna::profileIdFromString(elem.getAttribute("dlna:profileID")), value);
                    }
                    catch (std::exception&)
                    {
                        // no profile id present, add it as regular metadata
                        addPropertyToItem(key, value, item);
                    }
                }
                else
                {
                    addPropertyToItem(key, value, item);
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
                res.setSize(xml::utils::optionalStringToUnsignedNumeric<uint64_t>(attr->value_string()));
            }
            else if (strncmp("duration", attr->name(), attr->name_size()) == 0)
            {
                res.setDuration(durationFromString(attr->value_string()));
            }
            else if (strncmp("nrAudioChannels", attr->name(), attr->name_size()) == 0)
            {
                res.setNrAudioChannels(xml::utils::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("bitRate", attr->name(), attr->name_size()) == 0)
            {
                res.setBitRate(xml::utils::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("sampleFrequency", attr->name(), attr->name_size()) == 0)
            {
                res.setSampleRate(xml::utils::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
            }
            else if (strncmp("bitsPerSample", attr->name(), attr->name_size()) == 0)
            {
                res.setBitsPerSample(xml::utils::optionalStringToUnsignedNumeric<uint32_t>(attr->value_string()));
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
}
