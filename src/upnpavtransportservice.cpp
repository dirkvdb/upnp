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
#include "upnp.avtransport.typeconversions.h"

#include "utils/log.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "upnp/upnp.xml.parseutils.h"

namespace upnp
{
namespace AVTransport
{

using namespace utils;
using namespace rapidxml_ns;

static const char* s_ns             = "urn:schemas-upnp-org:event-1-0";
static const char* s_propertySet    = "e:propertyset";
static const char* s_property       = "e:property";
static const char* s_lastChange     = "LastChange";
static const char* s_xmlnsAtr       = "xmlns:e";
static const char* s_eventNode      = "Event";

static const char* s_valAtr         = "val";
static const char* s_instanceIdNode = "InstanceID";

Service::Service(IRootDevice& dev, IAVTransport& av)
: DeviceService(dev, ServiceType::AVTransport)
, m_avTransport(av)
, m_avTransport3(dynamic_cast<IAVTransport3*>(&av))
, m_LastChange(m_type, std::chrono::milliseconds(200))
{
    m_LastChange.LastChangeEvent = [this] (const xml::Document& doc) {
        m_rootDevice.notifyEvent(serviceTypeToUrnIdString(m_type), doc);
    };
}

Service::~Service()
{
    m_LastChange.LastChangeEvent = nullptr;
}

std::string Service::getSubscriptionResponse()
{
    xml_document<> doc;
    auto* propertySet = doc.allocate_node(node_element, s_propertySet);
    propertySet->append_attribute(doc.allocate_attribute(s_xmlnsAtr, s_ns));
    auto* property = doc.allocate_node(node_element, s_property);
    auto* lastChange = doc.allocate_node(node_element, s_lastChange);

    auto* event = doc.allocate_node(node_element, s_eventNode);
    event->append_attribute(doc.allocate_attribute(s_xmlnsAtr, serviceTypeToUrnMetadataString(m_type)));

    for (auto& vars : m_variables)
    {
        auto* instance = doc.allocate_node(node_element, s_instanceIdNode);
        auto* indexString = doc.allocate_string(std::to_string(vars.first).c_str());
        instance->append_attribute(doc.allocate_attribute(s_valAtr, indexString));

        for (auto& var : vars.second)
        {
            instance->append_node(xml::serviceVariableToElement(doc, var.second));
        }

        event->append_node(instance);
    }

    auto* eventString = doc.allocate_string(xml::encode(xml::toString(*event)).c_str());
    auto* lastChangeValue = doc.allocate_node(node_element, s_eventNode, eventString);

    lastChange->append_node(lastChangeValue);
    property->append_node(lastChange);
    propertySet->append_node(property);
    doc.append_node(propertySet);

    return xml::toString(doc);
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
        case Action::GetTransportInfo:
            response.addArgument("CurrentTransportState",   getInstanceVariable(id, Variable::TransportState).getValue());
            response.addArgument("CurrentTransportStatus",  getInstanceVariable(id, Variable::TransportStatus).getValue());
            response.addArgument("CurrentSpeed",            getInstanceVariable(id, Variable::TransportPlaySpeed).getValue());
            break;
        case Action::GetPositionInfo:
            response.addArgument("Track",                   getInstanceVariable(id, Variable::CurrentTrack).getValue());
            response.addArgument("TrackDuration",           getInstanceVariable(id, Variable::CurrentTrackDuration).getValue());
            response.addArgument("TrackMetaData",           getInstanceVariable(id, Variable::CurrentTrackMetaData).getValue());
            response.addArgument("TrackURI",                getInstanceVariable(id, Variable::CurrentTrackURI).getValue());
            response.addArgument("RelTime",                 getInstanceVariable(id, Variable::RelativeTimePosition).getValue());
            response.addArgument("AbsTime",                 getInstanceVariable(id, Variable::AbsoluteTimePosition).getValue());
            response.addArgument("RelCount",                getInstanceVariable(id, Variable::RelativeCounterPosition).getValue());
            response.addArgument("AbsCount",                getInstanceVariable(id, Variable::AbsoluteCounterPosition).getValue());
            break;
        case Action::GetDeviceCapabilities:
            response.addArgument("PlayMedia",               getInstanceVariable(id, Variable::PossiblePlaybackStorageMedia).getValue());
            response.addArgument("RecMedia",                getInstanceVariable(id, Variable::PossibleRecordStorageMedia).getValue());
            response.addArgument("RecQualityModes",         getInstanceVariable(id, Variable::PossibleRecordQualityModes).getValue());
            break;
        case Action::GetTransportSettings:
            response.addArgument("PlayMode",                getInstanceVariable(id, Variable::CurrentPlayMode).getValue());
            response.addArgument("RecQualityModes",         getInstanceVariable(id, Variable::CurrentRecordQualityMode).getValue());
            break;
        case Action::GetCurrentTransportActions:
            response.addArgument("Actions",                 getInstanceVariable(id, Variable::CurrentTransportActions).getValue());
            break;
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
        {
            auto val = req.getChildNodeValue("Unit");
            m_avTransport.seek(id, seekModeFromString(val), req.getChildNodeValue("Target"));
            break;
        }
        case Action::Next:
            m_avTransport.next(id);
            break;
        case Action::Previous:
            m_avTransport.previous(id);
            break;
        case Action::SetPlayMode:
        {
            auto val = req.getChildNodeValue("NewPlayMode");
            m_avTransport.setPlayMode(id, playModeFromString(val));
            break;
        }
        case Action::SetRecordQualityMode:
            m_avTransport.setRecordQualityMode(id, req.getChildNodeValue("￼NewRecordQualityMode"));
            break;
        // AVTransport:2
        case Action::GetMediaInfoExt:
            response.addArgument("CurrentType",             getInstanceVariable(id, Variable::CurrentMediaCategory).getValue());
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
        case Action::GetDRMState:
            response.addArgument("CurrentType",             getInstanceVariable(id, Variable::DRMState).getValue());
            break;
        case Action::GetStateVariables:
            response.addArgument("StateVariableList", getStateVariables(id, req.getChildNodeValue("StateVariableList")).toString());
            break;
        //case Action::SetStateVariables:
        //    break;

        // AVTransport:3
        case Action::GetSyncOffset:
            throwIfNoAVTransport3Support();
            response.addArgument("CurrentSyncOffset", getInstanceVariable(id, Variable::SyncOffset).getValue());
            break;
        case Action::AdjustSyncOffset:
            throwIfNoAVTransport3Support();
            m_avTransport3->adjustSyncOffset(id, req.getChildNodeValue("Adjustment"));
            break;
        case Action::SetSyncOffset:
            throwIfNoAVTransport3Support();
            m_avTransport3->setSyncOffset(id, req.getChildNodeValue("NewSyncOffset"));
            break;
        case Action::SyncPlay:
        {
            throwIfNoAVTransport3Support();
            auto val = req.getChildNodeValue("ReferencePositionUnits");
            m_avTransport3->syncPlay(id, req.getChildNodeValue("Speed"),
                                         seekModeFromString(val),
                                         req.getChildNodeValue("ReferencePosition"),
                                         req.getChildNodeValue("ReferencePresentationTime"),
                                         req.getChildNodeValue("ReferenceClockId"));
            break;
        }
        case Action::SyncStop:
            throwIfNoAVTransport3Support();
            m_avTransport3->syncStop(id, req.getChildNodeValue("StopTime"), req.getChildNodeValue("ReferenceClockId"));
            break;
        case Action::SyncPause:
            throwIfNoAVTransport3Support();
            m_avTransport3->syncPause(id, req.getChildNodeValue("PauseTime"), req.getChildNodeValue("ReferenceClockId"));
            break;
        case Action::SetStaticPlaylist:
            throwIfNoAVTransport3Support();
            m_avTransport3->setStaticPlaylist(id, req.getChildNodeValue("PlaylistData"),
                                                  static_cast<uint32_t>(std::stoul(req.getChildNodeValue("PlaylistOffset"))),
                                                  static_cast<uint32_t>(std::stoul(req.getChildNodeValue("PlaylistTotalLength"))),
                                                  req.getChildNodeValue("PlaylistMIMEType"),
                                                  req.getChildNodeValue("PlaylistExtendedType"),
                                                  req.getChildNodeValue("PlaylistStartObj"),
                                                  req.getChildNodeValue("PlaylistStartGroup"));
            break;
        case Action::SetStreamingPlaylist:
        {
            throwIfNoAVTransport3Support();
            auto val = req.getChildNodeValue("PlaylistStep");
            m_avTransport3->setStreamingPlaylist(id, req.getChildNodeValue("PlaylistData"),
                                                     req.getChildNodeValue("PlaylistMIMEType"),
                                                     req.getChildNodeValue("PlaylistExtendedType"),
                                                     playlistStepFromString(val));
            break;
        }
        case Action::GetPlaylistInfo:
        {
            auto val = req.getChildNodeValue("PlaylistType");
            throwIfNoAVTransport3Support();
            response.addArgument("PlaylistInfo", m_avTransport3->getPlaylistInfo(id, playlistTypeFromString(val)));
            break;
        }
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


    log::debug("Add change: {} {}", variableToString(var), value);
    m_LastChange.addChangedVariable(id, ServiceVariable(variableToString(var), value));
}

std::string Service::variableToString(Variable type) const
{
    return AVTransport::variableToString(type);
}

xml::Document Service::getStateVariables(uint32_t id, const std::string& variableList) const
{
    try
    {
        xml::Document doc;
        auto pairs = doc.createElement("stateVariableValuePairs");

        std::map<std::string, std::string> vars;
        if (variableList == "*")
        {
            vars = getVariables(id);
        }
        else
        {
            for (auto& var : csvToVector(variableList))
            {
                vars.insert(std::make_pair(var, getInstanceVariable(id, variableFromString(var)).getValue()));
            }
        }

        for (auto iter = vars.begin(); iter != vars.end();)
        {
            if (iter->first == "LastChange" || iter->first.find("A_ARG_TYPE_") == 0)
            {
                // lastchange and argtype variables are excluded
                continue;
            }

            auto var    = doc.createElement("stateVariable");
            auto value  = doc.createNode(iter->second);
            var.addAttribute("variableName", iter->first);
            var.appendChild(value);
            pairs.appendChild(var);
        }

        return doc;
    }
    catch (Exception& e)
    {
        throw InvalidStateVariableListException();
    }
}

void Service::throwIfNoAVTransport3Support()
{
    if (!m_avTransport3)
    {
        throw InvalidActionException();
    }
}

}
}
