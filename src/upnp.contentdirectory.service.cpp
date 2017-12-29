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

#include "upnp/upnp.contentdirectory.service.h"
#include "upnp.contentdirectory.typeconversions.h"

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

#include "upnp/upnp.item.h"
#include "upnp/upnp.servicefaults.h"
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml.hpp"

using namespace utils;

namespace upnp
{
namespace ContentDirectory
{

using namespace rapidxml_ns;

Service::Service(IRootDevice& dev, IContentDirectory& cd)
: DeviceService(dev, {ServiceType::ContentDirectory, 1})
, m_contentDirectory(cd)
{
}

std::string Service::getSubscriptionResponse()
{
    // TODO: avoid the copies
    std::vector<std::pair<std::string, std::string>> vars;
    vars.emplace_back(variableToString(Variable::TransferIDs), m_variables.at(0)[Variable::TransferIDs].getValue());
    vars.emplace_back(variableToString(Variable::SystemUpdateID), m_variables.at(0)[Variable::SystemUpdateID].getValue());
    vars.emplace_back(variableToString(Variable::ContainerUpdateIDs), m_variables.at(0)[Variable::ContainerUpdateIDs].getValue());

    auto doc = xml::createNotificationXml(vars);

#ifdef DEBUG_CONNECTION_MANAGER
    log::debug(doc);
#endif

    return doc;
}

ActionResponse Service::onAction(const std::string& action, const std::string& requestDoc)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(requestDoc.c_str());

    ActionResponse response(action, {ServiceType::ContentDirectory, 1});
    auto&          request = doc.first_node_ref();

    switch (actionFromString(action))
    {
    case Action::GetSearchCapabilities:
        response.addArgument("SearchCaps", getVariable(Variable::SearchCapabilities).getValue());
        break;
    case Action::GetSortCapabilities:
        response.addArgument("SortCaps", getVariable(Variable::SortCapabilities).getValue());
        break;
    case Action::GetSystemUpdateID:
        response.addArgument("Id", m_contentDirectory.GetSystemUpdateId());
        break;
    case Action::Browse:
    {
        auto id            = xml::requiredChildValue(request, "ObjectID");
        auto browseFlag    = xml::requiredChildValue(request, "BrowseFlag");
        auto flag          = browseFlagFromString(browseFlag);
        auto filterStrings = stringops::split(xml::requiredChildValue(request, "Filter"), ',');
        auto startIndex    = static_cast<uint32_t>(std::stoul(xml::requiredChildValue(request, "StartingIndex").c_str()));
        auto count         = static_cast<uint32_t>(std::stoul(xml::requiredChildValue(request, "RequestedCount").c_str()));
        auto sortStrings   = stringops::split(xml::requiredChildValue(request, "SortCriteria"), ',');

        std::vector<Property> filter;
        std::transform(filterStrings.begin(), filterStrings.end(), std::back_inserter(filter), [](const std::string& prop) {
            return propertyFromString(prop);
        });

        std::vector<SortProperty> sort;
        std::transform(sortStrings.begin(), sortStrings.end(), std::back_inserter(sort), [](const std::string& prop) -> SortProperty {
            if (prop.empty())
            {
                log::warn("Invalid sort property provided");
                throw InvalidArguments();
            }

            return SortProperty(propertyFromString(prop.substr(1)), sortTypeFromString(prop[0]));
        });

        auto res = m_contentDirectory.Browse(id, flag, filter, startIndex, count, sort);
        response.addArgument("Result", xml::getItemsDocument(res.result));
        response.addArgument("NumberReturned", std::to_string(res.numberReturned));
        response.addArgument("TotalMatches", std::to_string(res.totalMatches));
        response.addArgument("UpdateID", std::to_string(res.updateId));
        break;
    }
    case Action::Search:
    {
        auto id            = xml::requiredChildValue(request, "ContainerID");
        auto criteria      = xml::requiredChildValue(request, "SearchCriteria");
        auto filterStrings = stringops::split(xml::requiredChildValue(request, "Filter"), ',');
        auto startIndex    = static_cast<uint32_t>(std::stoul(xml::requiredChildValue(request, "StartingIndex").c_str()));
        auto count         = static_cast<uint32_t>(std::stoul(xml::requiredChildValue(request, "RequestedCount").c_str()));
        auto sortStrings   = stringops::split(xml::requiredChildValue(request, "SortCriteria"), ',');

        std::vector<Property> filter;
        std::transform(filterStrings.begin(), filterStrings.end(), std::back_inserter(filter), [](const std::string& prop) {
            return propertyFromString(prop);
        });

        std::vector<SortProperty> sort;
        std::transform(sortStrings.begin(), sortStrings.end(), std::back_inserter(sort), [](const std::string& prop) -> SortProperty {
            if (prop.empty())
            {
                log::warn("Invalid sort property provided");
                throw InvalidArguments();
            }

            return SortProperty(propertyFromString(prop.substr(1)), sortTypeFromString(prop[0]));
        });

        auto res = m_contentDirectory.Search(id, criteria, filter, startIndex, count, sort);
        response.addArgument("Result", xml::getItemsDocument(res.result));
        response.addArgument("NumberReturned", std::to_string(res.numberReturned));
        response.addArgument("TotalMatches", std::to_string(res.totalMatches));
        response.addArgument("UpdateID", std::to_string(res.updateId));
        break;
    }
    default:
        throw InvalidAction();
    }

    return response;
}

const char* Service::variableToString(Variable type) const
{
    return ContentDirectory::variableToString(type);
}
}
}
