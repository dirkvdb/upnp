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

#include "upnp/upnpcontentdirectoryservice.h"
#include "upnp.contentdirectory.typeconversions.h"

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

#include "upnp/upnp.item.h"
#include "upnp/upnp.xml.parseutils.h"

using namespace utils;

namespace upnp
{
namespace ContentDirectory
{

Service::Service(IRootDevice& dev, IContentDirectory& cd)
: DeviceService(dev, { ServiceType::ContentDirectory, 1 })
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

ActionResponse Service::onAction(const std::string& action, const xml::Document& doc)
{
    try
    {
        ActionResponse response(action, { ServiceType::ContentDirectory, 1});
        auto request = doc.getFirstChild();

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
            auto id = request.getChildNodeValue("ObjectID");
            auto browseFlag = request.getChildNodeValue("BrowseFlag");
            auto flag = browseFlagFromString(browseFlag);
            auto filterStrings = stringops::tokenize(request.getChildNodeValue("Filter"), ",");
            auto startIndex = static_cast<uint32_t>(std::stoul(request.getChildNodeValue("StartingIndex").c_str()));
            auto count = static_cast<uint32_t>(std::stoul(request.getChildNodeValue("RequestedCount").c_str()));
            auto sortStrings = stringops::tokenize(request.getChildNodeValue("SortCriteria"), ",");

            std::vector<Property> filter;
            std::transform(filterStrings.begin(), filterStrings.end(), std::back_inserter(filter), [] (const std::string& prop) {
                return propertyFromString(prop);
            });

            std::vector<SortProperty> sort;
            std::transform(sortStrings.begin(), sortStrings.end(), std::back_inserter(sort), [] (const std::string& prop) -> SortProperty {
                if (prop.empty())
                {
                    throw Exception("Invalid sort property provided");
                }

                return SortProperty(propertyFromString(prop.substr(1)), sortTypeFromString(prop[0]));
            });

            auto res = m_contentDirectory.Browse(id, flag, filter, startIndex, count, sort);
            response.addArgument("Result", xml::utils::getItemsDocument(res.result).toString());
            response.addArgument("NumberReturned", std::to_string(res.numberReturned));
            response.addArgument("TotalMatches", std::to_string(res.totalMatches));
            response.addArgument("UpdateID", std::to_string(res.updateId));
            break;
        }
        case Action::Search:
        {
            auto id = request.getChildNodeValue("ContainerID");
            auto criteria = request.getChildNodeValue("SearchCriteria");
            auto filterStrings = stringops::tokenize(request.getChildNodeValue("Filter"), ",");
            auto startIndex = static_cast<uint32_t>(std::stoul(request.getChildNodeValue("StartingIndex").c_str()));
            auto count = static_cast<uint32_t>(std::stoul(request.getChildNodeValue("RequestedCount").c_str()));
            auto sortStrings = stringops::tokenize(request.getChildNodeValue("SortCriteria"), ",");

            std::vector<Property> filter;
            std::transform(filterStrings.begin(), filterStrings.end(), std::back_inserter(filter), [] (const std::string& prop) {
                return propertyFromString(prop);
            });

            std::vector<SortProperty> sort;
            std::transform(sortStrings.begin(), sortStrings.end(), std::back_inserter(sort), [] (const std::string& prop) -> SortProperty {
                if (prop.empty())
                {
                    throw Exception("Invalid sort property provided");
                }

                return SortProperty(propertyFromString(prop.substr(1)), sortTypeFromString(prop[0]));
            });

            auto res = m_contentDirectory.Search(id, criteria, filter, startIndex, count, sort);
            response.addArgument("Result", xml::utils::getItemsDocument(res.result).toString());
            response.addArgument("NumberReturned", std::to_string(res.numberReturned));
            response.addArgument("TotalMatches", std::to_string(res.totalMatches));
            response.addArgument("UpdateID", std::to_string(res.updateId));
            break;
        }
        default:
            throw InvalidActionException();
        }

        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing ContentDirectory request: {}", e.what());
        throw InvalidActionException();
    }
}

const char* Service::variableToString(Variable type) const
{
    return ContentDirectory::variableToString(type);
}


}
}
