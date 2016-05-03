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

#pragma once

#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnp.connectionmanager.types.h"

namespace upnp
{

class Action;

namespace ConnectionManager
{

struct ServiceTraits
{
    using ActionType = ConnectionManager::Action;
    using VariableType = ConnectionManager::Variable;
    static const ServiceType SvcType = ServiceType::ConnectionManager;

    static ActionType actionFromString(const std::string& action);
    static const char* actionToString(ActionType action);
    static VariableType variableFromString(const std::string& var);
    static const char* variableToString(VariableType var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    Client(IClient2& client);

    void getProtocolInfo(std::function<void(int32_t, std::vector<ProtocolInfo>)> cb);
    void prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, int32_t peerConnectionId, Direction direction, std::function<void(int32_t, ConnectionInfo)> cb);
    void connectionComplete(const ConnectionInfo& connectionInfo, std::function<void(int32_t)> cb);
    void getCurrentConnectionIds(std::function<void(int32_t, std::vector<std::string>)> cb);
    void getCurrentConnectionInfo(int32_t connectionId, std::function<void(int32_t, ConnectionInfo)> cb);

protected:
    std::chrono::seconds getSubscriptionTimeout() override;
};

}
}
