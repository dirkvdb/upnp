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

#include "upnp/upnpavtransport.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnputils.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/numericoperations.h"

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

static const int32_t g_subscriptionTimeout = 1801;
    
AVTransport::AVTransport(IClient& client)
: ServiceBase(client)
{
}

void AVTransport::setAVTransportURI(const std::string& connectionId, const std::string& uri, const std::string& uriMetaData)
{
    executeAction(AVTransportAction::SetAVTransportURI, { {"InstanceID", connectionId},
                                                          {"CurrentURI", uri},
                                                          {"CurrentURIMetaData", uriMetaData} });
}

void AVTransport::play(const std::string& connectionId, const std::string& speed)
{
    executeAction(AVTransportAction::Play, { {"InstanceID", connectionId},
                                             {"Speed", speed} });
}

void AVTransport::pause(const std::string& connectionId)
{
    executeAction(AVTransportAction::Pause, { {"InstanceID", connectionId} });
}

void AVTransport::stop(const std::string& connectionId)
{
    executeAction(AVTransportAction::Stop, { {"InstanceID", connectionId} });
}

void AVTransport::next(const std::string& connectionId)
{
    executeAction(AVTransportAction::Next, { {"InstanceID", connectionId} });
}

void AVTransport::previous(const std::string& connectionId)
{
    executeAction(AVTransportAction::Previous, { {"InstanceID", connectionId} });
}

AVTransport::TransportInfo AVTransport::getTransportInfo(const std::string& connectionId)
{
    xml::Document doc = executeAction(AVTransportAction::GetTransportInfo, { {"InstanceID", connectionId} });
    
    TransportInfo info;
    info.currentTransportState      = doc.getChildElementValue("CurrentTransportState");
    info.currentTransportStatus     = doc.getChildElementValue("CurrentTransportStatus");
    info.currentSpeed               = doc.getChildElementValue("CurrentSpeed");
    
    return info;
}

AVTransport::PositionInfo AVTransport::getPositionInfo(const std::string& connectionId)
{
    xml::Document doc = executeAction(AVTransportAction::GetPositionInfo, { {"InstanceID", connectionId} });
    
    PositionInfo info;
    info.track          = doc.getChildElementValue("Track");
    info.trackDuration  = doc.getChildElementValue("TrackDuration");
    info.trackMetaData  = doc.getChildElementValue("TrackMetaData");
    info.trackURI       = doc.getChildElementValue("TrackURI");
    info.relTime        = doc.getChildElementValue("RelTime");
    info.absTime        = doc.getChildElementValue("AbsTime");
    info.relCount       = doc.getChildElementValue("RelCount");
    info.absCount       = doc.getChildElementValue("AbsCount");
    
    return info;
}

ServiceType AVTransport::getType()
{
    return ServiceType::AVTransport;
}

int32_t AVTransport::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}
    
AVTransportAction AVTransport::actionFromString(const std::string& action)
{
    if (action == "SetAVTransportURI")              return AVTransportAction::SetAVTransportURI;
    if (action == "SetNextAVTransportURI")          return AVTransportAction::SetNextAVTransportURI;
    if (action == "GetMediaInfo")                   return AVTransportAction::GetMediaInfo;
    if (action == "GetTransportInfo")               return AVTransportAction::GetTransportInfo;
    if (action == "GetPositionInfo")                return AVTransportAction::GetPositionInfo;
    if (action == "GetDeviceCapabilities")          return AVTransportAction::GetDeviceCapabilities;
    if (action == "GetTransportSettings")           return AVTransportAction::GetTransportSettings;
    if (action == "Stop")                           return AVTransportAction::Stop;
    if (action == "Play")                           return AVTransportAction::Play;
    if (action == "Pause")                          return AVTransportAction::Pause;
    if (action == "Record")                         return AVTransportAction::Record;
    if (action == "Seek")                           return AVTransportAction::Seek;
    if (action == "Next")                           return AVTransportAction::Next;
    if (action == "Previous")                       return AVTransportAction::Previous;
    if (action == "SetPlayMode")                    return AVTransportAction::SetPlayMode;
    if (action == "SetRecordQualityMode")           return AVTransportAction::SetRecordQualityMode;
    if (action == "GetCurrentTransportActions")     return AVTransportAction::GetCurrentTransportActions;

    throw std::logic_error("Unknown AVTransport action:" + action);
}

std::string AVTransport::actionToString(AVTransportAction action)
{
    switch (action)
    {
        case AVTransportAction::SetAVTransportURI:             return "SetAVTransportURI";
        case AVTransportAction::SetNextAVTransportURI:         return "SetNextAVTransportURI";
        case AVTransportAction::GetMediaInfo:                  return "GetMediaInfo";
        case AVTransportAction::GetTransportInfo:              return "GetTransportInfo";
        case AVTransportAction::GetPositionInfo:               return "GetPositionInfo";
        case AVTransportAction::GetDeviceCapabilities:         return "GetDeviceCapabilities";
        case AVTransportAction::GetTransportSettings:          return "GetTransportSettings";
        case AVTransportAction::Stop:                          return "Stop";
        case AVTransportAction::Play:                          return "Play";
        case AVTransportAction::Pause:                         return "Pause";
        case AVTransportAction::Record:                        return "Record";
        case AVTransportAction::Seek:                          return "Seek";
        case AVTransportAction::Next:                          return "Next";
        case AVTransportAction::Previous:                      return "Previous";  
        case AVTransportAction::SetPlayMode:                   return "SetPlayMode";
        case AVTransportAction::SetRecordQualityMode:          return "SetRecordQualityMode";
        case AVTransportAction::GetCurrentTransportActions:    return "GetCurrentTransportActions";
        default:
            throw std::logic_error("Invalid AVTransport action");
    }
}

AVTransportVariable AVTransport::variableFromString(const std::string& var)
{
    if (var == "TransportState")                 return AVTransportVariable::TransportState;
    if (var == "TransportStatus")                return AVTransportVariable::TransportStatus;
    if (var == "PlaybackStorageMedium")          return AVTransportVariable::PlaybackStorageMedium;
    if (var == "PossiblePlaybackStorageMedia")   return AVTransportVariable::PossiblePlaybackStorageMedia;
    if (var == "PossibleRecordStorageMedia")     return AVTransportVariable::PossibleRecordStorageMedia;
    if (var == "CurrentPlayMode")                return AVTransportVariable::CurrentPlayMode;
    if (var == "TransportPlaySpeed")             return AVTransportVariable::TransportPlaySpeed;
    if (var == "RecordStorageMedium")            return AVTransportVariable::RecordStorageMedium;
    if (var == "RecordMediumWriteStatus")        return AVTransportVariable::RecordMediumWriteStatus;
    if (var == "PossibleRecordQualityModes")     return AVTransportVariable::PossibleRecordQualityModes;
    if (var == "CurrentRecordQualityMode")       return AVTransportVariable::CurrentRecordQualityMode;
    if (var == "NumberOfTracks")                 return AVTransportVariable::NumberOfTracks;
    if (var == "CurrentTrack")                   return AVTransportVariable::CurrentTrack;
    if (var == "CurrentTrackDuration")           return AVTransportVariable::CurrentTrackDuration;
    if (var == "CurrentMediaDuration")           return AVTransportVariable::CurrentMediaDuration;
    if (var == "CurrentTrackURI")                return AVTransportVariable::CurrentTrackURI;
    if (var == "CurrentTrackMetaData")           return AVTransportVariable::CurrentTrackMetaData;
    if (var == "AVTransportURI")                 return AVTransportVariable::AVTransportURI;
    if (var == "AVTransportURIMetaData")         return AVTransportVariable::AVTransportURIMetaData;
    if (var == "NextAVTransportURI")             return AVTransportVariable::NextAVTransportURI;
    if (var == "NextAVTransportURIMetaData")     return AVTransportVariable::NextAVTransportURIMetaData;
    if (var == "CurrentTransportActions")        return AVTransportVariable::CurrentTransportActions;
    if (var == "RelativeTimePosition")           return AVTransportVariable::RelativeTimePosition;
    if (var == "AbsoluteTimePosition")           return AVTransportVariable::AbsoluteTimePosition;
    if (var == "RelativeCounterPosition")        return AVTransportVariable::RelativeCounterPosition;
    if (var == "AbsoluteCounterPosition")        return AVTransportVariable::AbsoluteCounterPosition;
    if (var == "A_ARG_TYPE_SeekMode")            return AVTransportVariable::ArgumentTypeSeekMode;
    if (var == "A_ARG_TYPE_SeekTarget")          return AVTransportVariable::ArgumentTypeSeekTarget;
    if (var == "A_ARG_TYPE_InstanceID")          return AVTransportVariable::ArgumentTypeInstanceId;
    if (var == "LastChange")                     return AVTransportVariable::LastChange;
    
    throw std::logic_error("Unknown AVTransport variable:" + var);
}

std::string AVTransport::variableToString(AVTransportVariable var)
{
    assert(false);
    throw std::logic_error("Unknown AVTransport variable");
}

void AVTransport::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 701: throw std::logic_error("Playback transition not supported at this moment");
        case 702: throw std::logic_error("No content found in media item");
        case 703: throw std::logic_error("The media could not be read");
        case 704: throw std::logic_error("Storage format not supported by the device");
        case 705: throw std::logic_error("The device is locked");
        case 706: throw std::logic_error("Error when writing media");
        case 707: throw std::logic_error("Media is not writable");
        case 708: throw std::logic_error("Format is not supported for recording");
        case 709: throw std::logic_error("The media is full");
        case 710: throw std::logic_error("Seek mode is not supported");
        case 711: throw std::logic_error("Illegal seek target");
        case 712: throw std::logic_error("Play mode is not supported");
        case 713: throw std::logic_error("Record quality is not supported");
        case 714: throw std::logic_error("Unsupported MIME-type");
        case 715: throw std::logic_error("Resource is already being played");
        case 716: throw std::logic_error("Resource not found");
        case 717: throw std::logic_error("Play speed not supported");
        case 718: throw std::logic_error("Invalid instance id");
        
        default: upnp::handleUPnPResult(errorCode);
    }
}
    
}
