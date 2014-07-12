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

#include <cassert>

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "utils/stringoperations.h"

using namespace utils;

//#define DEBUG_CONTENT_BROWSING

namespace upnp
{
namespace ContentDirectory
{

Service::Service(IRootDevice& dev, IContentDirectory& cd)
: DeviceService(dev, ServiceType::ContentDirectory)
, m_ContentDirectory(cd)
{
}

xml::Document Service::getSubscriptionResponse()
{
    const std::string ns = "urn:schemas-upnp-org:event-1-0";
    
    xml::Document doc;
    auto propertySet    = doc.createElement("e:propertyset");
    propertySet.addAttribute("xmlns:e", ns);
    
    //addPropertyToElement(0, Variable::SourceProtocolInfo, propertySet);
    //addPropertyToElement(0, Variable::SinkProtocolInfo, propertySet);
    //addPropertyToElement(0, Variable::CurrentConnectionIds, propertySet);
    
    doc.appendChild(propertySet);
    
#ifdef DEBUG_CONNECTION_MANAGER
    log::debug(doc.toString());
#endif
    
    return doc;
}

ActionResponse Service::onAction(const std::string& action, const xml::Document& doc)
{
    try
    {
        ActionResponse response(action, ServiceType::ContentDirectory);
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
                response.addArgument("Id", m_ContentDirectory.GetSystemUpdateId());
                break;
            case Action::Browse:
                break;
            default:
                throw InvalidActionException();
        }
        
        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing ContentDirectory request: %s", e.what());
        throw InvalidActionException();
    }
}



}
}
