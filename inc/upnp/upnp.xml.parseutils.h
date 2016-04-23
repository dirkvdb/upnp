#pragma once

#include <string>

#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpstatevariable.h"
#include "upnp/upnpcontentdirectorytypes.h"

#include "rapidxml.hpp"

namespace upnp
{

class Device;

namespace xml
{

std::string encode(const std::string& data);
std::string encode(const char* data, size_t dataSize);
std::string decode(const std::string& data);
std::string decode(const char* data, size_t dataSize);

void parseDeviceInfo(const std::string& xml, Device& device);
std::vector<StateVariable> getStateVariablesFromDescription(rapidxml_ns::xml_document<>& doc);
std::vector<std::string> getActionsFromDescription(rapidxml_ns::xml_document<>& doc);
std::map<std::string, std::string> getEventValues(rapidxml_ns::xml_document<>& doc);

Resource parseResource(rapidxml_ns::xml_node<>& node, const std::string& url);
Item parseContainer(rapidxml_ns::xml_node<>& containerElem);
std::vector<Item> parseContainers(const std::string& xml);
Item parseItem(rapidxml_ns::xml_node<>& itemElem);
std::vector<Item> parseItems(const std::string& xml);
Item parseMetaData(const std::string& meta);
std::string parseBrowseResult(const std::string& response, ContentDirectory::ActionResult& result);

template <typename T>
inline T optionalStringToUnsignedNumeric(const std::string& str)
{
    return str.empty() ? 0 : static_cast<T>(std::stoul(str));
}

template <typename T>
inline T optionalStringToNumeric(const std::string& str)
{
    return str.empty() ? 0 : static_cast<T>(std::stol(str));
}

}
}

