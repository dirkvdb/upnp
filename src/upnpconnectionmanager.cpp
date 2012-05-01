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

#include "upnp/upnpconnectionmanager.h"

#include "upnp/upnpclient.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpaction.h"
#include "upnp/upnputils.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

using namespace utils;

static const char* ConnectionManagerServiceType = "urn:schemas-upnp-org:service:ConnectionManager:1";

namespace upnp
{

ConnectionManager::ConnectionManager(const Client& client)
: m_Client(client)
{
}

void ConnectionManager::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
}

std::vector<ProtocolInfo> ConnectionManager::getProtocolInfo()
{
    std::vector<ProtocolInfo> protocolInfo;

    Action action("GetProtocolInfo", ConnectionManagerServiceType);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ConnectionManager].m_ControlURL.c_str(), ConnectionManagerServiceType, nullptr, action.getActionDocument(), &pResult));
    
    auto infos = stringops::tokenize(getFirstElementValue(pResult, "Sink"), ",");
    for (auto& info : infos)
    {
        try
        {
            protocolInfo.push_back(ProtocolInfo(info));
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
    }
    
    ixmlDocument_free(pResult);
    
    return protocolInfo;
}

ConnectionManager::ConnectionInfo ConnectionManager::prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionId, const std::string& peerConnectionManager, Direction direction)
{
    Action action("PrepareForConnection", ConnectionManagerServiceType);
	action.addArgument("RemoteProtocolInfo", protocolInfo.toString());
    action.addArgument("PeerConnectionManager", peerConnectionManager);
    action.addArgument("PeerConnectionID", peerConnectionId);
    action.addArgument("Direction", directionToString(direction));
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ConnectionManager].m_ControlURL.c_str(), ConnectionManagerServiceType, nullptr, action.getActionDocument(), &pResult));
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = getFirstElementValue(pResult, "ConnectionID");
    connInfo.avTransportId              = getFirstElementValue(pResult, "AVTransportID");
    connInfo.renderingControlServiceId  = getFirstElementValue(pResult, "RcsID");
    
    ixmlDocument_free(pResult);
    
    return connInfo;
}

void ConnectionManager::connectionComplete(const ConnectionInfo& connectionInfo)
{
    Action action("ConnectionComplete", ConnectionManagerServiceType);
    action.addArgument("ConnectionID", connectionInfo.connectionId);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ConnectionManager].m_ControlURL.c_str(), ConnectionManagerServiceType, nullptr, action.getActionDocument(), &pResult));
    
    ixmlDocument_free(pResult);
}

std::vector<std::string> ConnectionManager::getCurrentConnectionIds()
{
    Action action("GetCurrentConnectionIDs", ConnectionManagerServiceType);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ConnectionManager].m_ControlURL.c_str(), ConnectionManagerServiceType, nullptr, action.getActionDocument(), &pResult));

    std::vector<std::string> ids = stringops::tokenize(getFirstElementValue(pResult, "ConnectionIDs"), ",");
    
    ixmlDocument_free(pResult);
    
    return ids;
}

ConnectionManager::ConnectionInfo ConnectionManager::getCurrentConnectionInfo(const std::string& connectionId)
{
    Action action("GetCurrentConnectionInfo", ConnectionManagerServiceType);
	action.addArgument("ConnectionID", connectionId);
    
    IXML_Document* pResult = nullptr;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::ConnectionManager].m_ControlURL.c_str(), ConnectionManagerServiceType, nullptr, action.getActionDocument(), &pResult));
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = connectionId;
    connInfo.avTransportId              = getFirstElementValue(pResult, "AVTransportID");
    connInfo.renderingControlServiceId  = getFirstElementValue(pResult, "RcsID");
    connInfo.protocolInfo               = ProtocolInfo(getFirstElementValue(pResult, "ProtocolInfo"));
    connInfo.peerConnectionManager      = getFirstElementValue(pResult, "PeerConnectionManager");
    connInfo.peerConnectionId           = getFirstElementValue(pResult, "PeerConnectionID");
    connInfo.direction                  = directionFromString(getFirstElementValue(pResult, "Direction"));
    connInfo.connectionStatus           = connectionStatusFromString(getFirstElementValue(pResult, "Status"));
    
    ixmlDocument_free(pResult);
    
    return connInfo;
}

std::string ConnectionManager::directionToString(Direction direction)
{
    switch (direction)
    {
    case Direction::Input:      return "Input";
    case Direction::Output:     return "Output";
    default:                    throw std::logic_error("Invalid direction specified");
    }
}

ConnectionManager::Direction ConnectionManager::directionFromString(const std::string& direction)
{
    if (direction == "Input")   return Direction::Input;
    if (direction == "Output")  return Direction::Output;
    
    throw std::logic_error("Invalid direction received");
}

ConnectionManager::ConnectionStatus ConnectionManager::connectionStatusFromString(const std::string& status)
{
    if (status == "OK")                     return ConnectionStatus::Ok;
    if (status == "ContentFormatMismatch")  return ConnectionStatus::ContentFormatMismatch;
    if (status == "InsufficientBandwith")   return ConnectionStatus::InsufficientBandwith;
    if (status == "UnreliableChannel")      return ConnectionStatus::UnreliableChannel;
    
    return ConnectionStatus::Unknown;
}

}
