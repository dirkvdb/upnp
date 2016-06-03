//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#ifndef UPNP_CONNECTION_MANAGER_SERVICE_H
#define UPNP_CONNECTION_MANAGER_SERVICE_H

#include "upnp/upnpxml.h"
#include "upnp/upnpdeviceservice.h"
#include "upnp/upnp.connectionmanager.types.h"

#include <vector>
#include <string>

namespace upnp
{

class IConnectionManager
{
public:
    virtual ~IConnectionManager() {}
    virtual void prepareForConnection(const ProtocolInfo& protocolInfo, ConnectionManager::ConnectionInfo& info) = 0;
    virtual void connectionComplete(int32_t connectionId) = 0;
    virtual ConnectionManager::ConnectionInfo getCurrentConnectionInfo(int32_t connectionId) = 0;
};

class IConnectionManager3
{
public:
    virtual ~IConnectionManager3() {}
    virtual xml::Document getRendererItemInfo(const std::vector<std::string>& /*itemInfoFilter*/, const xml::Document& /*itemMetadataList*/) { throw InvalidActionException(); };
    virtual xml::Document getFeatureList() = 0;
};

namespace ConnectionManager
{

class Service : public DeviceService<Variable>
{
public:
    Service(IRootDevice& dev, IConnectionManager& cm);

    std::string getSubscriptionResponse() override;
    ActionResponse onAction(const std::string& action, const xml::Document& request) override;

protected:
    const char* variableToString(Variable type) const override;

private:
    void throwIfNoConnectionManager3Support();

    IConnectionManager&  m_connectionManager;
    IConnectionManager3* m_connectionManager3;
};

}
}

#endif
