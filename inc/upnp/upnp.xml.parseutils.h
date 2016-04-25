#pragma once

#include <string>

#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpstatevariable.h"
#include "upnp/upnpcontentdirectorytypes.h"



namespace rapidxml_ns
{

template<class Ch> class xml_node;
template<class Ch> class xml_document;

}

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
std::map<std::string, std::string> getEventValues(rapidxml_ns::xml_document<char>& doc);

Resource parseResource(rapidxml_ns::xml_node<char>& node, const std::string& url);
Item parseContainer(rapidxml_ns::xml_node<char>& containerElem);
std::vector<Item> parseContainers(const std::string& xml);
Item parseItem(rapidxml_ns::xml_node<char>& itemElem);
std::vector<Item> parseItems(const std::string& xml);
Item parseMetaData(const std::string& meta);
std::string parseBrowseResult(const std::string& response, ContentDirectory::ActionResult& result);
void parseEvent(const std::string& data, std::function<void(const std::string& varable, const std::map<std::string, std::string>&)> cb);
std::vector<StateVariable> parseServiceDescription(const std::string& contents, std::function<void(const std::string& action)> actionCb);

std::string optionalChildValue(rapidxml_ns::xml_node<char>& node, const char* child);

template <typename T>
inline T optionalStringToUnsignedNumeric(const std::string& str)
{
    try
    {
        return str.empty() ? 0 : static_cast<T>(std::stoul(str));
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument("Failed to convert string to integral: " + str);
    }
}

template <typename T>
inline T optionalStringToNumeric(const std::string& str)
{
    try
    {
        return str.empty() ? 0 : static_cast<T>(std::stol(str));
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument("Failed to convert string to integral: " + str);
    }
}

}
}

