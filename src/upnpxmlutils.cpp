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

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

#include <cstring>
#include <stdexcept>

using namespace utils;

namespace upnp
{
namespace xml
{
namespace utils
{

namespace
{

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
            elem.addAttribute("size", std::to_string(res.getSize()));
            elem.appendChild(node);
            itemElem.appendChild(elem);
        }
        catch (std::exception&) {}
    }

    didl.appendChild(itemElem);
}

}

std::vector<std::string> getActionsFromDescription(Document& doc)
{
    std::vector<std::string> actions;
    
    NodeList nodeList = doc.getElementsByTagName("action");
    uint64_t numActions = nodeList.size();
    actions.reserve(numActions);
    
    for (uint64_t i = 0; i < numActions; ++i)
    {
        Element actionElem = nodeList.getNode(i);
        actions.push_back(actionElem.getChildNodeValue("name"));
    }
    
    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(Document& doc)
{
    std::vector<StateVariable> variables;
    for (Element elem : doc.getElementsByTagName("stateVariable"))
    {
        try
        {
            StateVariable var;
            var.sendsEvents = elem.getAttribute("sendEvents") == "yes";
            var.name        = elem.getChildNodeValue("name");
            var.dataType    = elem.getChildNodeValue("dataType");
            
            try
            {
                Element rangeElement = elem.getElementsByTagName("allowedValueRange").getNode(0);
                std::unique_ptr<StateVariable::ValueRange> range(new StateVariable::ValueRange());
                
                range->minimumValue    = stringops::toNumeric<uint32_t>(rangeElement.getChildNodeValue("minimum"));
                range->maximumValue    = stringops::toNumeric<uint32_t>(rangeElement.getChildNodeValue("maximum"));
                range->step            = stringops::toNumeric<uint32_t>(rangeElement.getChildNodeValue("step"));
                
                var.valueRange = std::move(range);
            }
            catch(std::exception&) { /* no value range for this element, no biggy */ }
            
            variables.push_back(var);
        }
        catch(std::exception& e)
        {
            log::warn("Failed to parse state variable, skipping: %", e.what());
        }
    }
    
    return variables;
}

std::map<std::string, std::string> getEventValues(Document& doc)
{
    std::map<std::string, std::string> values;
    for (Element elem : doc.getChildNode("InstanceID").getChildNodes())
    {
        values.insert(std::make_pair(elem.getName(), elem.getAttribute("val")));
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

Document getItemsDocument(const std::vector<ItemPtr>& items)
{
    Document doc;
    auto didl = createDidlForDocument(doc);

    for (auto& item : items)
    {
        addItemToDidl(doc, didl, *item);
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
            else
            {
                res.addMetaData(key, value);
            }
        }
        catch (std::exception& e) { /* skip invalid resource */ log::warn(e.what()); }
    }
    
    return res;
}

static void addPropertyToItem(const std::string& propertyName, const std::string& propertyValue, const ItemPtr& item)
{
    Property prop = propertyFromString(propertyName);
    if (prop != Property::Unknown)
    {
        item->addMetaData(prop, propertyValue);
    }
    else
    {
        log::warn("Unknown property: %s", propertyName);
    }
}

ItemPtr parseItem(xml::Element& itemElem)
{
    auto item = std::make_shared<Item>();
    item->setObjectId(itemElem.getAttribute("id"));
    item->setParentId(itemElem.getAttribute("parentID"));

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
                    item->addResource(parseResource(nodeMap, value));
                }
                else if ("upnp:albumArtURI" == key)
                {
                    // multiple art uris can be present with different dlna profiles (size)
                    try
                    {
                        item->setAlbumArt(dlna::profileIdFromString(elem.getAttribute("dlna:profileID")), value);
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
            catch (std::exception& e) { /* try to parse the rest */ log::warn("Failed to parse upnp item: %s", e.what()); }
        }
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item");
    }
    
    return item;
}

ItemPtr parseItemDocument(Document& doc)
{
    auto elem = doc.getFirstChild();
    xml::Element itemElem = elem.getFirstChild();
    return parseItem(itemElem);
}

}
}
}
