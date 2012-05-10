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

#ifndef UPNP_CONTENT_DIRECTORY_H
#define UPNP_CONTENT_DIRECTORY_H

#include <string>
#include <memory>

#include <upnp/upnp.h>
#include "upnp/upnpitem.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpxmlutils.h"

#include "utils/subscriber.h"


namespace upnp
{

class Device;
class Client;

class ContentDirectory
{
public:
    enum BrowseType
    {
        All,
        ItemsOnly,
        ContainersOnly
    };
    
    struct SearchResult
    {
        uint32_t totalMatches;
        uint32_t numberReturned;
    };

    ContentDirectory(const Client& client);
        
    void setDevice(std::shared_ptr<Device> device);
    
    void abort();
    
    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;
    
    void browseMetadata(std::shared_ptr<Item>& item, const std::string& filter);
    void browseDirectChildren(BrowseType type, utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);
    SearchResult search(utils::ISubscriber<std::shared_ptr<Item>>& subscriber, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);
            
private:
    IXML_Document* browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);

    void querySearchCapabilities();
    void querySortCapabilities();
    void querySystemUpdateID();
    
    IXmlDocument parseBrowseResult(IXmlDocument& doc, SearchResult& result);
    void parseMetaData(IXmlDocument& doc, std::shared_ptr<Item>& item);
    std::vector<std::shared_ptr<Item>> parseContainers(IXmlDocument& doc);
    std::vector<std::shared_ptr<Item>> parseItems(IXmlDocument& doc);
    
    void notifySubscriber(std::vector<std::shared_ptr<Item>>& items, utils::ISubscriber<std::shared_ptr<Item>>& subscriber);

    const Client&               m_Client;
    std::shared_ptr<Device>     m_Device;
    
    std::vector<Property>       m_SearchCaps;
    std::vector<Property>       m_SortCaps;
    std::string                 m_SystemUpdateId;
    
    bool                        m_Abort;
};

}

#endif