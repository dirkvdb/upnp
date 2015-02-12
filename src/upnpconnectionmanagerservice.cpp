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

#include "upnp/upnpconnectionmanagerservice.h"
#include "utils/log.h"

using namespace utils;

namespace upnp
{
namespace ConnectionManager
{

Service::Service(IRootDevice& dev, IConnectionManager& cm)
: DeviceService(dev, ServiceType::ConnectionManager)
, m_connectionManager(cm)
, m_connectionManager3(dynamic_cast<IConnectionManager3*>(&cm))
{
}

xml::Document Service::getSubscriptionResponse()
{
    const std::string ns = "urn:schemas-upnp-org:event-1-0";

    xml::Document doc;
    auto propertySet    = doc.createElement("e:propertyset");
    propertySet.addAttribute("xmlns:e", ns);

    addPropertyToElement(0, Variable::SourceProtocolInfo, propertySet);
    addPropertyToElement(0, Variable::SinkProtocolInfo, propertySet);
    addPropertyToElement(0, Variable::CurrentConnectionIds, propertySet);
    
    doc.appendChild(propertySet);
    
#ifdef DEBUG_CONNECTION_MANAGER
    log::debug(doc.toString());
#endif
    
    return doc;
}

ActionResponse Service::onAction(const std::string& action, const xml::Document& doc)
{
    try
    {
        ActionResponse response(action, ServiceType::ConnectionManager);
        auto request = doc.getFirstChild();
    
        switch (actionFromString(action))
        {
        case Action::GetProtocolInfo:
            response.addArgument("Source",               getVariable(Variable::SourceProtocolInfo).getValue());
            response.addArgument("Sink",                 getVariable(Variable::SinkProtocolInfo).getValue());
            break;
        case Action::PrepareForConnection:
        {
            ConnectionInfo connInfo;
            connInfo.peerConnectionManager  = request.getChildNodeValue("PeerConnectionManager");
            connInfo.peerConnectionId       = std::stoi(request.getChildNodeValue("PeerConnectionID"));
            connInfo.direction              = directionFromString(request.getChildNodeValue("Direction"));
            
            ProtocolInfo protoInfo(request.getChildNodeValue("RemoteProtocolInfo"));;
            m_connectionManager.prepareForConnection(protoInfo, connInfo);
        
            response.addArgument("ConnectionID",         std::to_string(connInfo.connectionId));
            response.addArgument("AVTransportID",        std::to_string(connInfo.avTransportId));
            response.addArgument("RcsID",                std::to_string(connInfo.renderingControlServiceId));
            break;
        }
        case Action::ConnectionComplete:
            m_connectionManager.connectionComplete(std::stoi(request.getChildNodeValue("ConnectionID")));
            break;
        case Action::GetCurrentConnectionIDs:
            response.addArgument("ConnectionIDs",        getVariable(Variable::CurrentConnectionIds).getValue());
            break;
        case Action::GetCurrentConnectionInfo:
        {
            auto connInfo = m_connectionManager.getCurrentConnectionInfo(std::stoi(request.getChildNodeValue("ConnectionID")));
            response.addArgument("RcsID",                   std::to_string(connInfo.renderingControlServiceId));
            response.addArgument("AVTransportID",           std::to_string(connInfo.avTransportId));
            response.addArgument("ProtocolInfo",            connInfo.protocolInfo.toString());
            response.addArgument("PeerConnectionManager",   connInfo.peerConnectionManager);
            response.addArgument("PeerConnectionID",        std::to_string(connInfo.peerConnectionId));
            response.addArgument("Direction",               toString(connInfo.direction));
            response.addArgument("Status",                  toString(connInfo.connectionStatus));
            break;
        }
        
        case Action::GetRendererItemInfo:
            throwIfNoConnectionManager3Support();
            response.addArgument("ItemRenderingInfoList", m_connectionManager3->getRendererItemInfo(
                                    csvToVector(request.getChildNodeValue("ItemInfoFilter")),
                                    xml::Document(request.getChildNodeValue("ItemMetadataList"))).toString());
            break;
        case Action::GetFeatureList:
            throwIfNoConnectionManager3Support();
            response.addArgument("FeatureList", m_connectionManager3->getFeatureList().toString());
            break;
        
        default:
            throw InvalidActionException();
        }
        
        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing ConnectionManager request: {}", e.what());
        throw InvalidActionException();
    }
}

std::string Service::variableToString(Variable type) const
{
    return ConnectionManager::toString(type);
}

void Service::throwIfNoConnectionManager3Support()
{
    if (!m_connectionManager3)
    {
        throw InvalidActionException();
    }
}


}
}
