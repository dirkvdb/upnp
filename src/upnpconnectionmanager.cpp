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

#include "utils/log.h"
#include "utils/stringoperations.h"

using namespace utils;


namespace upnp
{

std::string ConnectionManager::UnknownConnectionId = "-1";
std::string ConnectionManager::DefaultConnectionId = "0";

ConnectionManager::ConnectionManager(const Client& client)
: m_Client(client)
{
}

void ConnectionManager::setDevice(const std::shared_ptr<Device>& device)
{
    if (device->implementsService(ServiceType::ConnectionManager))
    {
        m_Service = device->m_Services[ServiceType::ConnectionManager];
        parseServiceDescription(m_Service.m_SCPDUrl);
    }
}

bool ConnectionManager::supportsAction(Action action) const
{
    return m_SupportedActions.find(action) != m_SupportedActions.end();
}

std::vector<ProtocolInfo> ConnectionManager::getProtocolInfo()
{
    std::vector<ProtocolInfo> protocolInfo;

    upnp::Action action("GetProtocolInfo", m_Service.m_ControlURL, ServiceType::ConnectionManager);
    IXmlDocument result = sendAction(action);
    
    auto infos = stringops::tokenize(getFirstElementValue(result, "Sink"), ",");
    for (auto& info : infos)
    {
        try
        {
            protocolInfo.push_back(ProtocolInfo(info));
            log::debug(info);
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
        
        protocolInfo.push_back(ProtocolInfo("http-get:*:audio/m3u:*"));
    }
    
    return protocolInfo;
}

ConnectionInfo ConnectionManager::prepareForConnection(const ProtocolInfo& protocolInfo, const std::string& peerConnectionId, const std::string& peerConnectionManager, Direction direction)
{
    upnp::Action action("PrepareForConnection", m_Service.m_ControlURL, ServiceType::ConnectionManager);
	action.addArgument("RemoteProtocolInfo", protocolInfo.toString());
    action.addArgument("PeerConnectionManager", peerConnectionManager);
    action.addArgument("PeerConnectionID", peerConnectionId);
    action.addArgument("Direction", directionToString(direction));
    
    IXmlDocument result = sendAction(action);
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = getFirstElementValue(result, "ConnectionID");
    connInfo.avTransportId              = getFirstElementValue(result, "AVTransportID");
    connInfo.renderingControlServiceId  = getFirstElementValue(result, "RcsID");
    
    return connInfo;
}

void ConnectionManager::connectionComplete(const ConnectionInfo& connectionInfo)
{
    upnp::Action action("ConnectionComplete", m_Service.m_ControlURL, ServiceType::ConnectionManager);
    action.addArgument("ConnectionID", connectionInfo.connectionId);
    
    sendAction(action);
}

std::vector<std::string> ConnectionManager::getCurrentConnectionIds()
{
    upnp::Action action("GetCurrentConnectionIDs", m_Service.m_ControlURL, ServiceType::ConnectionManager);
    
    IXmlDocument result = sendAction(action);
    std::vector<std::string> ids = stringops::tokenize(getFirstElementValue(result, "ConnectionIDs"), ",");
    
    return ids;
}

ConnectionInfo ConnectionManager::getCurrentConnectionInfo(const std::string& connectionId)
{
    upnp::Action action("GetCurrentConnectionInfo", m_Service.m_ControlURL, ServiceType::ConnectionManager);
	action.addArgument("ConnectionID", connectionId);
    
    IXmlDocument result = sendAction(action);
    
    ConnectionInfo connInfo;
    connInfo.connectionId               = connectionId;
    connInfo.avTransportId              = getFirstElementValue(result, "AVTransportID");
    connInfo.renderingControlServiceId  = getFirstElementValue(result, "RcsID");
    connInfo.protocolInfo               = ProtocolInfo(getFirstElementValue(result, "ProtocolInfo"));
    connInfo.peerConnectionManager      = getFirstElementValue(result, "PeerConnectionManager");
    connInfo.peerConnectionId           = getFirstElementValue(result, "PeerConnectionID");
    connInfo.direction                  = directionFromString(getFirstElementValue(result, "Direction"));
    connInfo.connectionStatus           = connectionStatusFromString(getFirstElementValue(result, "Status"));
    
    return connInfo;
}

void ConnectionManager::parseServiceDescription(const std::string& descriptionUrl)
{
    IXmlDocument doc;
    
    int ret = UpnpDownloadXmlDoc(descriptionUrl.c_str(), &doc);
    if (ret != UPNP_E_SUCCESS)
    {
        log::error("Error obtaining device description from", descriptionUrl, " error =", ret);
        return;
    }
    
    for (auto& action : getActionsFromDescription(doc))
    {
        try
        {
            m_SupportedActions.insert(actionFromString(action));
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    }
}

ConnectionManager::Action ConnectionManager::actionFromString(const std::string& action)
{
    if (action == "GetProtocolInfo")            return Action::GetProtocolInfo;
    if (action == "PrepareForConnection")       return Action::PrepareForConnection;
    if (action == "ConnectionComplete")         return Action::ConnectionComplete;
    if (action == "GetCurrentConnectionIDs")    return Action::GetCurrentConnectionIDs;
    if (action == "GetCurrentConnectionInfo")   return Action::GetCurrentConnectionInfo;
    
    throw std::logic_error("Unknown ConnectionManager action:" + action);
}

void ConnectionManager::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 701: throw std::logic_error("Incompatible protocol info");
        case 702: throw std::logic_error("Incompatible directions");
        case 703: throw std::logic_error("Insufficient network resources");
        case 704: throw std::logic_error("Local restrictions");
        case 705: throw std::logic_error("Access denied");
        case 706: throw std::logic_error("Invalid connection reference");
        case 707: throw std::logic_error("Managers are not part of the same network");
        default: upnp::handleUPnPResult(errorCode);
    }
}

IXmlDocument ConnectionManager::sendAction(const upnp::Action& action)
{
    try
    {
        return m_Client.sendAction(action);
    }
    catch (int32_t errorCode)
    {
        handleUPnPResult(errorCode);
    }
    
    assert(false);
    return IXmlDocument();
}

}
