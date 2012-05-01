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

#ifndef UPNP_CONNECTION_MANAGER_H
#define UPNP_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <memory>

#include "upnp/upnpprotocolinfo.h"

namespace upnp
{

class Device;
class Client;
    
class ConnectionManager
{
public:
    enum class Direction
    {
        Input,
        Output
    };
    
    enum class ConnectionStatus
    {
        Ok,
        ContentFormatMismatch,
        InsufficientBandwith,
        UnreliableChannel,
        Unknown
    };
    
    struct ConnectionInfo
    {
        std::string         connectionId;
        std::string         avTransportId;
        std::string         renderingControlServiceId;
        ProtocolInfo        protocolInfo;
        std::string         peerConnectionManager;
        std::string         peerConnectionId;
        Direction           direction;
        ConnectionStatus    connectionStatus;
    };

    ConnectionManager(const Client& cp);
    
    void setDevice(std::shared_ptr<Device> device);
    
    std::vector<ProtocolInfo> getProtocolInfo();
    ConnectionInfo prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionManager, const std::string& peerConnectionId, Direction direction);
    void connectionComplete(const ConnectionInfo& connectionInfo);
    std::vector<std::string> getCurrentConnectionIds();
    ConnectionInfo getCurrentConnectionInfo(const std::string& connectionId);
    
private:
    static std::string directionToString(Direction direction);
    static Direction directionFromString(const std::string& direction);
    static ConnectionStatus connectionStatusFromString(const std::string& status);

    const Client&               m_Client;
    std::shared_ptr<Device>     m_Device;
};
    
}

#endif