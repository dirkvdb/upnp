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

#include "upnp/upnpavtransportservice.h"
#include "utils/log.h"

using namespace utils;

namespace upnp
{
namespace AVTransport
{

Service::Service(IRootDevice& dev, IAVTransport& av)
: DeviceService(dev, ServiceType::AVTransport)
, m_avTransport(av)
, m_LastChange(m_Type, std::chrono::milliseconds(200))
{
    m_LastChange.LastChangeEvent.connect([this] (const xml::Document& doc) {
        m_RootDevice.notifyEvent(serviceTypeToUrnIdString(m_Type), doc);
    }, this);
}

Service::~Service()
{
    m_LastChange.LastChangeEvent.disconnect(this);
}

xml::Document Service::getSubscriptionResponse()
{
    const std::string ns = "urn:schemas-upnp-org:event-1-0";

    xml::Document doc;
    auto propertySet    = doc.createElement("e:propertyset");
    auto property       = doc.createElement("e:property");
    auto lastChange     = doc.createElement("LastChange");
    
    propertySet.addAttribute("xmlns:e", ns);
    
    std::stringstream ss;
    ss << "<Event xmlns=\"" << serviceTypeToUrnMetadataString(m_Type) << "\">" << std::endl;
    
    for (auto& vars : m_Variables)
    {
        ss << "<InstanceID val=\"" << vars.first << "\">";
    
        for (auto& var : vars.second)
        {
            ss << var.second.toString();
        }
        
        ss << "</InstanceID>";
    }
    
    ss << "</Event>";


    auto lastChangeValue = doc.createNode(ss.str());

    lastChange.appendChild(lastChangeValue);
    property.appendChild(lastChange);
    propertySet.appendChild(property);
    doc.appendChild(propertySet);
    
    log::debug(doc.toString());
    
    return doc;
}


ActionResponse Service::onAction(const std::string& action, const xml::Document& request)
{
    try
    {
        ActionResponse response(action, ServiceType::AVTransport);
        uint32_t id = std::stoul(request.getChildNodeValueRecursive("InstanceID"));
    
        switch (actionFromString(action))
        {
        case Action::SetAVTransportURI:
            m_avTransport.setAVTransportURI(id, request.getChildNodeValue("CurrentURI"), request.getChildNodeValue("CurrentURIMetaData"));
            break;
        case Action::SetNextAVTransportURI:
            m_avTransport.setNextAVTransportURI(id, request.getChildNodeValue("NextURI"), request.getChildNodeValue("NextURIMetaData"));
            break;
        case Action::GetMediaInfo:
        {
            response.addArgument("NrTracks",                getInstanceVariable(id, Variable::NumberOfTracks).getValue());
            response.addArgument("MediaDuration",           getInstanceVariable(id, Variable::CurrentMediaDuration).getValue());
            response.addArgument("CurrentURI",              getInstanceVariable(id, Variable::CurrentTrackURI).getValue());
            response.addArgument("CurrentURIMetaData",      getInstanceVariable(id, Variable::CurrentTrackMetaData).getValue());
            response.addArgument("NextURI",                 getInstanceVariable(id, Variable::NextAVTransportURI).getValue());
            response.addArgument("NextURIMetaData",         getInstanceVariable(id, Variable::NextAVTransportURIMetaData).getValue());
            response.addArgument("PlayMedium",              getInstanceVariable(id, Variable::PlaybackStorageMedium).getValue());
            response.addArgument("RecordMedium",            getInstanceVariable(id, Variable::RecordStorageMedium).getValue());
            response.addArgument("WriteStatus",             getInstanceVariable(id, Variable::RecordMediumWriteStatus).getValue());
            break;
        }
        case Action::GetTransportInfo:
        {
            response.addArgument("CurrentTransportState",   getInstanceVariable(id, Variable::TransportState).getValue());
            response.addArgument("CurrentTransportStatus",  getInstanceVariable(id, Variable::TransportStatus).getValue());
            response.addArgument("CurrentSpeed",            getInstanceVariable(id, Variable::TransportPlaySpeed).getValue());
            break;
        }
        case Action::GetPositionInfo:
        {
            response.addArgument("Track",                   getInstanceVariable(id, Variable::CurrentTrack).getValue());
            response.addArgument("TrackDuration",           getInstanceVariable(id, Variable::CurrentTrackDuration).getValue());
            response.addArgument("TrackMetaData",           getInstanceVariable(id, Variable::CurrentTrackMetaData).getValue());
            response.addArgument("TrackURI",                getInstanceVariable(id, Variable::CurrentTrackURI).getValue());
            response.addArgument("RelTime",                 getInstanceVariable(id, Variable::RelativeTimePosition).getValue());
            response.addArgument("AbsTime",                 getInstanceVariable(id, Variable::AbsoluteTimePosition).getValue());
            response.addArgument("RelCount",                getInstanceVariable(id, Variable::RelativeCounterPosition).getValue());
            response.addArgument("AbsCount",                getInstanceVariable(id, Variable::AbsoluteCounterPosition).getValue());
            break;
        }
        case Action::GetDeviceCapabilities:
        {
            response.addArgument("PlayMedia",               getInstanceVariable(id, Variable::PossiblePlaybackStorageMedia).getValue());
            response.addArgument("RecMedia",                getInstanceVariable(id, Variable::PossibleRecordStorageMedia).getValue());
            response.addArgument("RecQualityModes",         getInstanceVariable(id, Variable::PossibleRecordQualityModes).getValue());
            break;
        }
        case Action::GetTransportSettings:
        {
            response.addArgument("PlayMode",                getInstanceVariable(id, Variable::CurrentPlayMode).getValue());
            response.addArgument("RecQualityModes",         getInstanceVariable(id, Variable::CurrentRecordQualityMode).getValue());
            break;
        }
        case Action::GetCurrentTransportActions:
        {
            response.addArgument("Actions",                 getInstanceVariable(id, Variable::CurrentTransportActions).getValue());
            break;
        }
        case Action::Stop:
            m_avTransport.stop(id);
            break;
        case Action::Play:
            m_avTransport.play(id, request.getChildNodeValue("Speed"));
            break;
        case Action::Pause:
            m_avTransport.pause(id);
            break;
        case Action::Record:
            m_avTransport.record(id);
            break;
        case Action::Seek:
            m_avTransport.seek(id, seekModeFromString(request.getChildNodeValue("Unit")), request.getChildNodeValue("Target"));
            break;
        case Action::Next:
            m_avTransport.next(id);
            break;
        case Action::Previous:
            m_avTransport.previous(id);
            break;
        case Action::SetPlayMode:
            m_avTransport.setPlayMode(id, playModeFromString(request.getChildNodeValue("NewPlayMode")));
            break;
        case Action::SetRecordQualityMode:
            m_avTransport.setRecordQualityMode(id, request.getChildNodeValue("￼NewRecordQualityMode"));
            break;
        default:
            throw InvalidActionException();
        }
        
        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing request: %s", e.what());
        throw InvalidActionException();
    }
}

std::string Service::variableToString(Variable type) const
{
    return AVTransport::variableToString(type);
}

}
}
