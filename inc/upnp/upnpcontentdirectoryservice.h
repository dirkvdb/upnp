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

#ifndef UPNP_CONTENT_DIRECTORY_SERVICE_H
#define UPNP_CONTENT_DIRECTORY_SERVICE_H

#include "upnp/upnpdeviceservice.h"
#include "upnp/upnp.contentdirectory.types.h"

namespace upnp
{

class Action;
class Device;
class IClient;

typedef std::function<void(const Item&)> ItemCb;

namespace ContentDirectory
{

class IContentDirectory
{
public:
    virtual ~IContentDirectory() {}

    // Required
    virtual std::vector<Property> GetSearchCapabilities() = 0;
    virtual std::vector<Property> GetSortCapabilities() = 0;
    virtual std::string GetSystemUpdateId() = 0;
    virtual ActionResult Browse(const std::string& id, BrowseFlag flag, const std::vector<Property>& filter, uint32_t startIndex, uint32_t count, const std::vector<SortProperty>& sortCriteria) = 0;

    // Optional
    virtual ActionResult Search(const std::string& /*id*/, const std::string& /*criteria*/, const std::vector<Property>& /*filter*/, uint32_t /*startIndex*/, uint32_t /*count*/, const std::vector<SortProperty>& /*sortCriteria*/)        { throw InvalidActionException(); }
    virtual void CreateObject()          { throw InvalidActionException(); }
    virtual void DestroyObject()         { throw InvalidActionException(); }
    virtual void UpdateObject()          { throw InvalidActionException(); }
    virtual void ImportResource()        { throw InvalidActionException(); }
    virtual void ExportResource()        { throw InvalidActionException(); }
    virtual void StopTransferResource()  { throw InvalidActionException(); }
    virtual void GetTransferProgress()   { throw InvalidActionException(); }
};

class Service : public DeviceService<Variable>
{
public:
    Service(IRootDevice& dev, IContentDirectory& cd);

    xml::Document getSubscriptionResponse() override;
    ActionResponse onAction(const std::string& action, const xml::Document& request) override;

protected:
    std::string variableToString(Variable type) const override;

private:
    IContentDirectory&          m_contentDirectory;
};

}
}

#endif
