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
    m_LastChange.LastChangeEvent = [this] (const xml::Document& doc) {
        m_RootDevice.notifyEvent(serviceTypeToUrnIdString(m_Type), doc);
    };
}

Service::~Service()
{
    m_LastChange.LastChangeEvent = nullptr;
}

xml::Document Service::getSubscriptionResponse()
{
    const std::string ns = "urn:schemas-upnp-org:event-1-0";

    xml::Document doc;
    auto propertySet    = doc.createElement("e:propertyset");
    auto property       = doc.createElement("e:property");
    auto lastChange     = doc.createElement("LastChange");
    
    propertySet.addAttribute("xmlns:e", ns);
    
    auto event = doc.createElement("Event");
    event.addAttribute("xmlns", serviceTypeToUrnMetadataString(m_Type));
    
    for (auto& vars : m_Variables)
    {
        auto instance = doc.createElement("InstanceID");
        instance.addAttribute("val", std::to_string(vars.first));

        for (auto& var : vars.second)
        {
            auto elem = xml::utils::serviceVariableToElement(doc, var.second);
            instance.appendChild(elem);
        }

        event.appendChild(instance);
    }
    
    auto lastChangeValue = doc.createNode(event.toString());

    lastChange.appendChild(lastChangeValue);
    property.appendChild(lastChange);
    propertySet.appendChild(property);
    doc.appendChild(propertySet);

#ifdef DEBUG_AVTRANSPORT
    log::debug(doc.toString());
#endif
    
    return doc;
}


ActionResponse Service::onAction(const std::string& action, const xml::Document& doc)
{
    try
    {
        ActionResponse response(action, ServiceType::AVTransport);
        auto req = doc.getFirstChild();
        uint32_t id = static_cast<uint32_t>(std::stoul(req.getChildNodeValue("InstanceID")));
        
        switch (actionFromString(action))
        {
        case Action::SetAVTransportURI:
            m_avTransport.setAVTransportURI(id, req.getChildNodeValue("CurrentURI"), req.getChildNodeValue("CurrentURIMetaData"));
            break;
        case Action::SetNextAVTransportURI:
            m_avTransport.setNextAVTransportURI(id, req.getChildNodeValue("NextURI"), req.getChildNodeValue("NextURIMetaData"));
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
            m_avTransport.play(id, req.getChildNodeValue("Speed"));
            break;
        case Action::Pause:
            m_avTransport.pause(id);
            break;
        case Action::Record:
            m_avTransport.record(id);
            break;
        case Action::Seek:
            m_avTransport.seek(id, seekModeFromString(req.getChildNodeValue("Unit")), req.getChildNodeValue("Target"));
            break;
        case Action::Next:
            m_avTransport.next(id);
            break;
        case Action::Previous:
            m_avTransport.previous(id);
            break;
        case Action::SetPlayMode:
            m_avTransport.setPlayMode(id, playModeFromString(req.getChildNodeValue("NewPlayMode")));
            break;
        case Action::SetRecordQualityMode:
            m_avTransport.setRecordQualityMode(id, req.getChildNodeValue("￼NewRecordQualityMode"));
            break;
        default:
            throw InvalidActionException();
        }
        
        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing request: {}", e.what());
        throw InvalidActionException();
    }
}

void Service::setInstanceVariable(uint32_t id, Variable var, const std::string& value)
{
    if (getInstanceVariable(id, var).getValue() == value)
    {
        // value is the same
        return;
    }

    DeviceService::setInstanceVariable(id, var, value);

    if (var == Variable::RelativeTimePosition ||
        var == Variable::AbsoluteTimePosition ||
        var == Variable::RelativeCounterPosition ||
        var == Variable::AbsoluteCounterPosition)
    {
        // these variable are not added to the LastChange variable
        return;
    }
    
    
    log::debug("Add change: {} {}", toString(var), value);
    m_LastChange.addChangedVariable(id, ServiceVariable(toString(var), value));
}

std::string Service::variableToString(Variable type) const
{
    return AVTransport::toString(type);
}

}
}
