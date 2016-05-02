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

#ifndef UPNP_TEST_UTILS_H
#define UPNP_TEST_UTILS_H

#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"

#include "utils/numericoperations.h"

using namespace utils;

namespace upnp
{

namespace
{

inline void addResponseHeader(std::ostream& ss, const std::string& action, ServiceType type)
{
    ss << "<u:" << action << "Response xmlns:u=\"" << serviceTypeToUrnTypeString(type) << "\">" << std::endl;
}

inline void addResponseFooter(std::ostream& ss, const std::string& action)
{
    ss << "</u:" << action << "Response>";
}

}

inline std::string generateActionResponse(const std::string& action, ServiceType type, const std::vector<std::pair<std::string, std::string>>& vars = {})
{
    std::stringstream ss;
    addResponseHeader(ss, action, type);

    for (auto& var : vars)
    {
        ss << "    <" << var.first << ">" << var.second << "</" << var.first << ">" << std::endl;
    }

    addResponseFooter(ss, action);

    return ss.str();
}

inline std::string wrapSoap(const std::string& actionResponse)
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?>" << std::endl
       << "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
       << "<s:Body>"
       << actionResponse
       << "</s:Body>"
       << "</s:Envelope>";
    return ss.str();
}

inline std::string generateBrowseResponse(const std::vector<upnp::Item>& containers, const std::vector<upnp::Item>& items)
{
    std::stringstream ss;
    ss << "&lt;DIDL-Lite xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; xmlns:upnp=&quot;urn:schemas-upnp-org:metadata-1-0/upnp/&quot; xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot; xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot;&gt;" << std::endl;

    for (auto& item : containers)
    {
        ss << "&lt;container id=&quot;" << item.getObjectId() << "&quot; parentID=&quot;" << item.getParentId() << "&quot; restricted=&quot;1&quot; childCount=&quot;" << item.getChildCount() << "&quot;&gt;"
           << "&lt;dc:title&gt;" << item.getTitle() << "&lt;/dc:title&gt;";

        for (auto& meta : item.getMetaData())
        {
            ss << "&lt;" << toString(meta.first) << "&gt;" << meta.second << "&lt;/" << toString(meta.first) << "&gt;";
        }

        ss << "&lt;/container&gt;";
    }

    for (auto& item : items)
    {
        ss << "&lt;item id=&quot;" << item.getObjectId() << "&quot; parentID=&quot;" << item.getParentId() << "&quot; restricted=&quot;1&quot;&gt;"
           << "&lt;dc:title&gt;" << item.getTitle() << "&lt;/dc:title&gt;";

        for (auto& meta : item.getMetaData())
        {
            ss << "&lt;" << toString(meta.first) << "&gt;" << meta.second << "&lt;/" << toString(meta.first) << "&gt;";
        }

        ss << "&lt;/item&gt;";
    }

    ss << "&lt;/DIDL-Lite&gt;";

    return wrapSoap(generateActionResponse("Browse", ServiceType::ContentDirectory, { std::make_pair("Result", ss.str()),
                                                                                      std::make_pair("NumberReturned", numericops::toString(items.size())),
                                                                                      std::make_pair("TotalMatches", numericops::toString(items.size())),
                                                                                      std::make_pair("UpdateID", "1")}));
}

}

#endif
