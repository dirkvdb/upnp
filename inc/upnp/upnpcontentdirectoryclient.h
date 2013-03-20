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

#ifndef UPNP_CONTENT_DIRECTORY_CLIENT_H
#define UPNP_CONTENT_DIRECTORY_CLIENT_H

#include "upnp/upnpitem.h"
#include "upnp/upnpservicebase.h"
#include "upnp/upnpcontentdirectorytypes.h"

namespace upnp
{

class Action;
class Device;
class IClient;

typedef std::function<void(const ItemPtr&)> ItemCb;

namespace ContentDirectory
{

class Client : public ServiceBase<Action, Variable>
{
public:
    enum BrowseType
    {
        All,
        ItemsOnly,
        ContainersOnly
    };
    
    struct ActionResult
    {
        uint32_t totalMatches;
        uint32_t numberReturned;
    };

    Client(IClient& client);
        
    void setDevice(const std::shared_ptr<Device>& device);
    
    void abort();
    
    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;
    
    void browseMetadata(const ItemPtr& item, const std::string& filter);
    ActionResult browseDirectChildren(BrowseType type, const ItemCb& onItem, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);
    ActionResult search(const ItemCb& onItem, const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);
    
protected:
    virtual Action actionFromString(const std::string& action);
    virtual std::string actionToString(Action action);
    virtual Variable variableFromString(const std::string& var);
    virtual std::string variableToString(Variable var);

    virtual ServiceType getType();
    virtual int32_t getSubscriptionTimeout();
    virtual void handleUPnPResult(int errorCode);
            
private:
    xml::Document browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort);

    void querySearchCapabilities();
    void querySortCapabilities();
    void querySystemUpdateID();
    
    static void addPropertyToItem(const std::string& propertyName, const std::string& propertyValue, const std::shared_ptr<Item>& item);
    static void addPropertyToList(const std::string& propertyName, std::vector<Property>& vec);
    
    xml::Document parseBrowseResult(xml::Document& doc, ActionResult& result);
    void parseMetaData(xml::Document& doc, const std::shared_ptr<Item>& item);
    
    void parseContainer(xml::Element& containerElem, const std::shared_ptr<Item>& item);
    void parseItem(xml::Element& itemElem, const std::shared_ptr<Item>& item);
    Resource parseResource(xml::NamedNodeMap& nodeMap, const std::string& url);
    
    std::vector<std::shared_ptr<Item>> parseContainers(xml::Document& doc);
    std::vector<std::shared_ptr<Item>> parseItems(xml::Document& doc);
    
    void notifySubscriber(std::vector<std::shared_ptr<Item>>& items, const ItemCb& onItem);

    std::vector<Property>       m_SearchCaps;
    std::vector<Property>       m_SortCaps;
    std::string                 m_SystemUpdateId;
    
    bool                        m_Abort;
};

}
}

#endif
