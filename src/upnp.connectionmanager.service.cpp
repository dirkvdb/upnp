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

#include "upnp/upnp.connectionmanager.service.h"
#include "upnp.connectionmanager.typeconversions.h"

#include "utils/log.h"
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml.hpp"

using namespace utils;

namespace upnp
{
namespace ConnectionManager
{

using namespace rapidxml_ns;

Service::Service(IRootDevice& dev, IConnectionManager& cm)
: DeviceService(dev, { ServiceType::ConnectionManager, 1 })
, m_connectionManager(cm)
, m_connectionManager3(dynamic_cast<IConnectionManager3*>(&cm))
{
}

std::string Service::getSubscriptionResponse()
{
    // TODO: avoid the copies
    std::vector<std::pair<std::string, std::string>> vars;
    vars.emplace_back(variableToString(Variable::SourceProtocolInfo), m_variables.at(0)[Variable::SourceProtocolInfo].getValue());
    vars.emplace_back(variableToString(Variable::SinkProtocolInfo), m_variables.at(0)[Variable::SinkProtocolInfo].getValue());
    vars.emplace_back(variableToString(Variable::CurrentConnectionIds), m_variables.at(0)[Variable::CurrentConnectionIds].getValue());

    auto doc = xml::createNotificationXml(vars);

#ifdef DEBUG_CONNECTION_MANAGER
    log::debug(doc);
#endif

    return doc;
}

ActionResponse Service::onAction(const std::string& action, const std::string& requestXml)
{
    xml_document<> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(requestXml.c_str());

    ActionResponse response(action, { ServiceType::ConnectionManager, 1 });
    auto& request = doc.first_node_ref().first_node_ref().first_node_ref();

    switch (actionFromString(action))
    {
    case Action::GetProtocolInfo:
        response.addArgument("Source",               getVariable(Variable::SourceProtocolInfo).getValue());
        response.addArgument("Sink",                 getVariable(Variable::SinkProtocolInfo).getValue());
        break;
    case Action::PrepareForConnection:
    {
        ConnectionInfo connInfo;
        connInfo.peerConnectionManager  = xml::requiredChildValue(request, "PeerConnectionManager");
        connInfo.peerConnectionId       = std::stoi(xml::requiredChildValue(request, "PeerConnectionID"));
        connInfo.direction              = directionFromString(xml::requiredChildValue(request, "Direction"));

        ProtocolInfo protoInfo(xml::requiredChildValue(request, "RemoteProtocolInfo"));
        m_connectionManager.prepareForConnection(protoInfo, connInfo);

        response.addArgument("ConnectionID",         std::to_string(connInfo.connectionId));
        response.addArgument("AVTransportID",        std::to_string(connInfo.avTransportId));
        response.addArgument("RcsID",                std::to_string(connInfo.renderingControlServiceId));
        break;
    }
    case Action::ConnectionComplete:
        m_connectionManager.connectionComplete(std::stoi(xml::requiredChildValue(request, "ConnectionID")));
        break;
    case Action::GetCurrentConnectionIDs:
        response.addArgument("ConnectionIDs", getVariable(Variable::CurrentConnectionIds).getValue());
        break;
    case Action::GetCurrentConnectionInfo:
    {
        auto connInfo = m_connectionManager.getCurrentConnectionInfo(std::stoi(xml::requiredChildValue(request, "ConnectionID")));
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
        // TODO: CHeck ItemMetadataList argument (wrap in xml doxument?)
        throwIfNoConnectionManager3Support();
        response.addArgument("ItemRenderingInfoList", m_connectionManager3->getRendererItemInfo(
                                csvToVector(xml::requiredChildValue(request, "ItemInfoFilter")),
                                xml::requiredChildValue(request, "ItemMetadataList")));
        break;
    case Action::GetFeatureList:
        throwIfNoConnectionManager3Support();
        response.addArgument("FeatureList", m_connectionManager3->getFeatureList());
        break;

    default:
        throw InvalidAction();
    }

    return response;
}

const char* Service::variableToString(Variable type) const
{
    return ConnectionManager::toString(type);
}

void Service::throwIfNoConnectionManager3Support()
{
    if (!m_connectionManager3)
    {
        throw InvalidAction();
    }
}


}
}
