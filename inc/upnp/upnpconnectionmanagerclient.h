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

#include "upnp/upnpservicebase.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnpconnectionmanagertypes.h"

namespace upnp
{

class Action;

namespace ConnectionManager
{

class Client : public ServiceBase<Action, Variable>
{
public:    
    Client(IClient& client);
    
    std::vector<ProtocolInfo> getProtocolInfo();
    ConnectionInfo prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, int32_t peerConnectionId, Direction direction);
    void connectionComplete(const ConnectionInfo& connectionInfo);
    std::vector<std::string> getCurrentConnectionIds();
    ConnectionInfo getCurrentConnectionInfo(int32_t connectionId);
    
protected:
    virtual Action actionFromString(const std::string& action);
    virtual std::string actionToString(Action action);
    virtual Variable variableFromString(const std::string& var);
    virtual std::string variableToString(Variable var);

    ServiceType getType();
    int32_t getSubscriptionTimeout();
    
    void handleUPnPResult(int errorCode);
};
    
}
}

#endif