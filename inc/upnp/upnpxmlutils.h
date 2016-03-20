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

#ifndef UPNP_XML_UTILS_H
#define UPNP_XML_UTILS_H

#include <map>
#include <string>
#include <vector>
#include <cassert>

#include "upnp/upnpxml.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpstatevariable.h"
#include "utils/stringoperations.h"

namespace upnp
{

class Item;
class Resource;
class ServiceVariable;

namespace xml
{
namespace utils
{

std::vector<StateVariable> getStateVariablesFromDescription(Document& doc);
std::vector<std::string> getActionsFromDescription(Document& doc);
std::map<std::string, std::string> getEventValues(Document& doc);
Document getItemDocument(const Item& item);
Document getItemsDocument(const std::vector<Item>& item);

Element createServiceVariablesElement(Document& doc, uint32_t instanceId, const std::vector<ServiceVariable>& vars);
Element serviceVariableToElement(Document& doc, const ServiceVariable& var);

Resource parseResource(xml::NamedNodeMap& nodeMap, const std::string& url);
Item parseItem(xml::Element& itemElem);
Item parseItemDocument(Document& doc);

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
}

#endif
