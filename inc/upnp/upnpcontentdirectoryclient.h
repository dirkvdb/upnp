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
#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnpcontentdirectorytypes.h"

namespace upnp
{

class Action;
class Device;
class IClient2;

typedef std::function<void(const Item&)> ItemCb;

namespace ContentDirectory
{

class Client : public ServiceClientBase<Action, Variable>
{
public:
    enum BrowseType
    {
        All,
        ItemsOnly,
        ContainersOnly
    };

    Client(upnp::IClient2& client);

    void setDevice(const std::shared_ptr<Device>& device) override;

    void abort();

    const std::vector<Property>& getSearchCapabilities() const;
    const std::vector<Property>& getSortCapabilities() const;

    void browseMetadata(const std::string& objectId, const std::string& filter, const std::function<void(int32_t, Item)> cb);
    void browseDirectChildren(BrowseType type, const std::string& objectId, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb);
    void search(const std::string& objectId, const std::string& criteria, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, const std::function<void(int32_t, ActionResult)> cb);

protected:
    virtual Action actionFromString(const std::string& action) const override;
    virtual std::string actionToString(Action action) const override;
    virtual Variable variableFromString(const std::string& var) const override;
    virtual std::string variableToString(Variable var) const override;

    virtual ServiceType getType() override;
    virtual std::chrono::seconds getSubscriptionTimeout() override;
    virtual void handleUPnPResult(int errorCode) override;

private:
    void browseAction(const std::string& objectId, const std::string& flag, const std::string& filter, uint32_t startIndex, uint32_t limit, const std::string& sort, std::function<void(int32_t, std::string)> cb);

    void querySearchCapabilities();
    void querySortCapabilities();
    void querySystemUpdateID();

    std::vector<Property>       m_searchCaps;
    std::vector<Property>       m_sortCaps;
    std::string                 m_systemUpdateId;

    bool                        m_abort;
};

}
}

#endif
