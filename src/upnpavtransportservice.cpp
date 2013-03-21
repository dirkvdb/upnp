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

namespace upnp
{
namespace AVTransport
{

Service::Service(IAVTransport& av)
: DeviceService(ServiceType::AVTransport)
, m_avTransport(av)
{
}

ActionResponse Service::onAction(const std::string& action, const xml::Document& request)
{
    ActionResponse response(action, ServiceType::AVTransport);
    uint32_t id = std::stoul(request.getChildNodeValue("InstanceID"));
    
    try
    {
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
            auto info = m_avTransport.getMediaInfo(id);
            response.addArgument("NrTracks",            std::to_string(info.numberOfTracks));
            response.addArgument("MediaDuration",       info.mediaDuration);
            response.addArgument("CurrentURI",          info.currentURI);
            response.addArgument("CurrentURIMetaData",  info.currentURIMetaData);
            response.addArgument("NextURI",             info.nextURI);
            response.addArgument("NextURIMetaData",     info.nextURIMetaData);
            response.addArgument("PlayMedium",          info.playMedium);
            response.addArgument("RecordMedium",        info.recordMedium);
            response.addArgument("WriteStatus",         info.writeStatus);
            break;
        }
        case Action::GetTransportInfo:
        {
            auto info = m_avTransport.getTransportInfo(id);
            response.addArgument("CurrentTransportState",   stateToString(info.currentTransportState));
            response.addArgument("CurrentTransportStatus",  statusToString(info.currentTransportStatus));
            response.addArgument("CurrentSpeed",            info.currentSpeed);
            break;
        }
        case Action::GetPositionInfo:
        {
            auto info = m_avTransport.getPositionInfo(id);
            response.addArgument("Track",               std::to_string(info.track));
            response.addArgument("TrackDuration",       info.trackDuration);
            response.addArgument("TrackMetaData",       info.trackMetaData);
            response.addArgument("TrackURI",            info.trackURI);
            response.addArgument("RelTime",             info.relativeTime);
            response.addArgument("AbsTime",             info.absoluteTime);
            response.addArgument("RelCount",            std::to_string(info.relativeCount));
            response.addArgument("AbsCount",            std::to_string(info.absoluteCount));
            break;
        }
        case Action::GetDeviceCapabilities:
        {
            auto info = m_avTransport.getDeviceCapabilities(id);
            response.addArgument("PlayMedia",           info.playMedia);
            response.addArgument("RecMedia",            info.recordMedia);
            response.addArgument("RecQualityModes",     info.recordQualityModes);
            break;
        }
        case Action::GetTransportSettings:
        {
            auto info = m_avTransport.getTransportSettings(id);
            response.addArgument("PlayMode",           info.playMode);
            response.addArgument("RecQualityMode",     info.recordQualityMode);
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
        case Action::GetCurrentTransportActions:
        {
            response.addArgument("PlayMode", vectorToCSV<Action>(m_avTransport.getCurrentTransportActions(id), &AVTransport::actionToString));
            break;
        }
        default:
            throw InvalidActionException();
        }
    }
    catch (std::exception&)
    {
        throw InvalidActionException();
    }
    
    return response;
}

}
}