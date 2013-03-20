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

#ifndef UPNP_AV_TRANSPORT_TYPES_H
#define UPNP_AV_TRANSPORT_TYPES_H

namespace upnp
{
namespace AVTransport
{
    
enum class Action
{
    SetAVTransportURI,
    SetNextAVTransportURI, // Optional
    GetMediaInfo,
    GetTransportInfo,
    GetPositionInfo,
    GetDeviceCapabilities,
    GetTransportSettings,
    Stop,
    Play,
    Pause, // Optional
    Record, // Optional
    Seek,
    Next,
    Previous,
    SetPlayMode, // Optional
    SetRecordQualityMode, // Optional
    GetCurrentTransportActions // Optional
};

enum class Variable
{
    TransportState,
    TransportStatus,
    PlaybackStorageMedium,
    PossiblePlaybackStorageMedia,
    PossibleRecordStorageMedia,
    CurrentPlayMode,
    TransportPlaySpeed,
    RecordStorageMedium,
    RecordMediumWriteStatus,
    PossibleRecordQualityModes,
    CurrentRecordQualityMode,
    NumberOfTracks,
    CurrentTrack,
    CurrentTrackDuration,
    CurrentMediaDuration,
    CurrentTrackURI,
    CurrentTrackMetaData,
    AVTransportURI,
    AVTransportURIMetaData,
    NextAVTransportURI,
    NextAVTransportURIMetaData,
    CurrentTransportActions,
    RelativeTimePosition,
    AbsoluteTimePosition,
    RelativeCounterPosition,
    AbsoluteCounterPosition,
    ArgumentTypeSeekMode,
    ArgumentTypeSeekTarget,
    ArgumentTypeInstanceId,
    LastChange //event
};

enum class State
{
    Stopped,
    Playing,
    Transitioning,
    PausedPlayback,
    PausedRecording,
    Recording,
    NoMediaPresent
};

inline Action actionFromString(const std::string& action)
{
    if (action == "SetAVTransportURI")              return Action::SetAVTransportURI;
    if (action == "SetNextAVTransportURI")          return Action::SetNextAVTransportURI;
    if (action == "GetMediaInfo")                   return Action::GetMediaInfo;
    if (action == "GetTransportInfo")               return Action::GetTransportInfo;
    if (action == "GetPositionInfo")                return Action::GetPositionInfo;
    if (action == "GetDeviceCapabilities")          return Action::GetDeviceCapabilities;
    if (action == "GetTransportSettings")           return Action::GetTransportSettings;
    if (action == "Stop")                           return Action::Stop;
    if (action == "Play")                           return Action::Play;
    if (action == "Pause")                          return Action::Pause;
    if (action == "Record")                         return Action::Record;
    if (action == "Seek")                           return Action::Seek;
    if (action == "Next")                           return Action::Next;
    if (action == "Previous")                       return Action::Previous;
    if (action == "SetPlayMode")                    return Action::SetPlayMode;
    if (action == "SetRecordQualityMode")           return Action::SetRecordQualityMode;
    if (action == "GetCurrentTransportActions")     return Action::GetCurrentTransportActions;

    throw std::logic_error("Unknown AVTransport action:" + action);
}

inline std::string actionToString(Action action)
{
    switch (action)
    {
        case Action::SetAVTransportURI:             return "SetAVTransportURI";
        case Action::SetNextAVTransportURI:         return "SetNextAVTransportURI";
        case Action::GetMediaInfo:                  return "GetMediaInfo";
        case Action::GetTransportInfo:              return "GetTransportInfo";
        case Action::GetPositionInfo:               return "GetPositionInfo";
        case Action::GetDeviceCapabilities:         return "GetDeviceCapabilities";
        case Action::GetTransportSettings:          return "GetTransportSettings";
        case Action::Stop:                          return "Stop";
        case Action::Play:                          return "Play";
        case Action::Pause:                         return "Pause";
        case Action::Record:                        return "Record";
        case Action::Seek:                          return "Seek";
        case Action::Next:                          return "Next";
        case Action::Previous:                      return "Previous";
        case Action::SetPlayMode:                   return "SetPlayMode";
        case Action::SetRecordQualityMode:          return "SetRecordQualityMode";
        case Action::GetCurrentTransportActions:    return "GetCurrentTransportActions";
        default:
            throw std::logic_error("Invalid AVTransport action");
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "TransportState")                 return Variable::TransportState;
    if (var == "TransportStatus")                return Variable::TransportStatus;
    if (var == "PlaybackStorageMedium")          return Variable::PlaybackStorageMedium;
    if (var == "PossiblePlaybackStorageMedia")   return Variable::PossiblePlaybackStorageMedia;
    if (var == "PossibleRecordStorageMedia")     return Variable::PossibleRecordStorageMedia;
    if (var == "CurrentPlayMode")                return Variable::CurrentPlayMode;
    if (var == "TransportPlaySpeed")             return Variable::TransportPlaySpeed;
    if (var == "RecordStorageMedium")            return Variable::RecordStorageMedium;
    if (var == "RecordMediumWriteStatus")        return Variable::RecordMediumWriteStatus;
    if (var == "PossibleRecordQualityModes")     return Variable::PossibleRecordQualityModes;
    if (var == "CurrentRecordQualityMode")       return Variable::CurrentRecordQualityMode;
    if (var == "NumberOfTracks")                 return Variable::NumberOfTracks;
    if (var == "CurrentTrack")                   return Variable::CurrentTrack;
    if (var == "CurrentTrackDuration")           return Variable::CurrentTrackDuration;
    if (var == "CurrentMediaDuration")           return Variable::CurrentMediaDuration;
    if (var == "CurrentTrackURI")                return Variable::CurrentTrackURI;
    if (var == "CurrentTrackMetaData")           return Variable::CurrentTrackMetaData;
    if (var == "AVTransportURI")                 return Variable::AVTransportURI;
    if (var == "AVTransportURIMetaData")         return Variable::AVTransportURIMetaData;
    if (var == "NextAVTransportURI")             return Variable::NextAVTransportURI;
    if (var == "NextAVTransportURIMetaData")     return Variable::NextAVTransportURIMetaData;
    if (var == "CurrentTransportActions")        return Variable::CurrentTransportActions;
    if (var == "RelativeTimePosition")           return Variable::RelativeTimePosition;
    if (var == "AbsoluteTimePosition")           return Variable::AbsoluteTimePosition;
    if (var == "RelativeCounterPosition")        return Variable::RelativeCounterPosition;
    if (var == "AbsoluteCounterPosition")        return Variable::AbsoluteCounterPosition;
    if (var == "A_ARG_TYPE_SeekMode")            return Variable::ArgumentTypeSeekMode;
    if (var == "A_ARG_TYPE_SeekTarget")          return Variable::ArgumentTypeSeekTarget;
    if (var == "A_ARG_TYPE_InstanceID")          return Variable::ArgumentTypeInstanceId;
    if (var == "LastChange")                     return Variable::LastChange;
    
    throw std::logic_error("Unknown AVTransport variable:" + var);
}

inline std::string variableToString(Variable var)
{
    switch (var)
    {
    case Variable::TransportState:                   return "TransportState";
    case Variable::TransportStatus:                  return "TransportStatus";
    case Variable::PlaybackStorageMedium:            return "PlaybackStorageMedium";
    case Variable::PossiblePlaybackStorageMedia:     return "PossiblePlaybackStorageMedia";
    case Variable::PossibleRecordStorageMedia:       return "PossibleRecordStorageMedia";
    case Variable::CurrentPlayMode:                  return "CurrentPlayMode";
    case Variable::TransportPlaySpeed:               return "TransportPlaySpeed";
    case Variable::RecordStorageMedium:              return "RecordStorageMedium";
    case Variable::RecordMediumWriteStatus:          return "RecordMediumWriteStatus";
    case Variable::PossibleRecordQualityModes:       return "PossibleRecordQualityModes";
    case Variable::CurrentRecordQualityMode:         return "CurrentRecordQualityMode";
    case Variable::NumberOfTracks:                   return "NumberOfTracks";
    case Variable::CurrentTrack:                     return "CurrentTrack";
    case Variable::CurrentTrackDuration:             return "CurrentTrackDuration";
    case Variable::CurrentMediaDuration:             return "CurrentMediaDuration";
    case Variable::CurrentTrackURI:                  return "CurrentTrackURI";
    case Variable::CurrentTrackMetaData:             return "CurrentTrackMetaData";
    case Variable::AVTransportURI:                   return "AVTransportURI";
    case Variable::AVTransportURIMetaData:           return "AVTransportURIMetaData";
    case Variable::NextAVTransportURI:               return "NextAVTransportURI";
    case Variable::NextAVTransportURIMetaData:       return "NextAVTransportURIMetaData";
    case Variable::CurrentTransportActions:          return "CurrentTransportActions";
    case Variable::RelativeTimePosition:             return "RelativeTimePosition";
    case Variable::AbsoluteTimePosition:             return "AbsoluteTimePosition";
    case Variable::RelativeCounterPosition:          return "RelativeCounterPosition";
    case Variable::AbsoluteCounterPosition:          return "AbsoluteCounterPosition";
    case Variable::ArgumentTypeSeekMode:             return "ArgumentTypeSeekMode";
    case Variable::ArgumentTypeSeekTarget:           return "ArgumentTypeSeekTarget";
    case Variable::ArgumentTypeInstanceId:           return "ArgumentTypeInstanceId";
    case Variable::LastChange:                       return "LastChange";
    default:
        throw std::logic_error("Unknown AVTransport variable");
    }    
}

}
}

#endif
