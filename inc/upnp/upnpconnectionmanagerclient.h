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

#ifndef UPNP_CONNECTION_MANAGER_CLIENT_H
#define UPNP_CONNECTION_MANAGER_CLIENT_H

#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpconnectionmanagertypes.h"

namespace upnp
{

class Action;

namespace ConnectionManager
{

class Client : public ServiceClientBase<Action, Variable>
{
public:    
    Client(Client2& client);
    
    void getProtocolInfo(std::function<void(int32_t, std::vector<ProtocolInfo>)> cb);
    void prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, int32_t peerConnectionId, Direction direction, std::function<void(int32_t, ConnectionInfo)> cb);
    void connectionComplete(const ConnectionInfo& connectionInfo, std::function<void(int32_t)> cb);
    void getCurrentConnectionIds(std::function<void(int32_t, std::vector<std::string>)> cb);
    void getCurrentConnectionInfo(int32_t connectionId, std::function<void(int32_t, ConnectionInfo)> cb);
    
protected:
    virtual Action actionFromString(const std::string& action) const override;
    virtual std::string actionToString(Action action) const override;
    virtual Variable variableFromString(const std::string& var) const override;
    virtual std::string variableToString(Variable var) const override;

    ServiceType getType() override;
    std::chrono::seconds getSubscriptionTimeout() override;
    
    void handleUPnPResult(int errorCode) override;
};
    
}
}

#endif