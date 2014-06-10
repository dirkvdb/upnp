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

#include "upnp/upnpdeviceserviceexceptions.h"

namespace upnp
{
namespace AVTransport
{

DEFINE_UPNP_SERVICE_EXCEPTION(InvalidInstanceIdException,       "Invalid instance id",      702)
DEFINE_UPNP_SERVICE_EXCEPTION(IllegalMimeTypeException,         "Illegal MIME type",        714)
DEFINE_UPNP_SERVICE_EXCEPTION(ResourceNotFoundException,        "Resource not found",       716)
    
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

enum class Status
{
    Ok,
    Error
};

enum class SeekMode
{
    TrackNumber,
    AbsoluteTime,
    RelativeTime,
    AbsoluteCount,
    RelativeCount,
    ChannelFrequency,
    TapeIndex,
    Frame
};

enum class PlayMode
{
    Normal,
    Shuffle,
    RepeatOne,
    RepeatAll,
    Random,
    Direct,
    Intro
};
    
struct TransportInfo
{
    State       currentTransportState;
    Status      currentTransportStatus;
    std::string currentSpeed;
};

struct MediaInfo
{
    uint32_t    numberOfTracks;
    std::string mediaDuration;
    std::string currentURI;
    std::string currentURIMetaData;
    std::string nextURI;
    std::string nextURIMetaData;
    std::string playMedium;
    std::string recordMedium;
    std::string writeStatus;
};

struct PositionInfo
{
    uint32_t    track;
    std::string trackDuration;
    std::string trackMetaData;
    std::string trackURI;
    std::string relativeTime;
    std::string absoluteTime;
    int32_t     relativeCount;
    int32_t     absoluteCount;
};

struct DeviceCapabilities
{
    std::string playMedia;
    std::string recordMedia;
    std::string recordQualityModes;
};

struct TransportSettings
{
    std::string playMode;
    std::string recordQualityMode;
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

inline std::string toString(Action action)
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

inline std::string toString(Variable var)
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

inline State stateFromString(const std::string& state)
{
    if (state == "STOPPED")             return State::Stopped;
    if (state == "PLAYING")             return State::Playing;
    if (state == "TRANSITIONING")       return State::Transitioning;
    if (state == "PAUSED_PLAYBACK")     return State::PausedPlayback;
    if (state == "PAUSED_RECORDING")    return State::PausedRecording;
    if (state == "RECORDING")           return State::Recording;
    if (state == "NO_MEDIA_PRESENT")    return State::NoMediaPresent;

    throw std::logic_error("Unknown AVTransport state:" + state);
}

inline std::string toString(State state)
{
    switch (state)
    {
        case State::Stopped:            return "STOPPED";
        case State::Playing:            return "PLAYING";
        case State::Transitioning:      return "TRANSITIONING";
        case State::PausedPlayback:     return "PAUSED_PLAYBACK";
        case State::PausedRecording:    return "PAUSED_RECORDING";
        case State::Recording:          return "RECORDING";
        case State::NoMediaPresent:     return "NO_MEDIA_PRESENT";
        default:
            throw std::logic_error("Invalid AVTransport state");
    }
}

inline Status statusFromString(const std::string& status)
{
    if (status == "OK")                 return Status::Ok;
    if (status == "ERROR_OCCURRED")     return Status::Error;

    throw std::logic_error("Unknown AVTransport status:" + status);
}

inline std::string toString(Status status)
{
    switch (status)
    {
        case Status::Ok:            return "OK";
        case Status::Error:         return "ERROR_OCCURRED";
        default:
            throw std::logic_error("Invalid AVTransport status");
    }
}

inline SeekMode seekModeFromString(const std::string& mode)
{
    if (mode == "TRACK_NR")        return SeekMode::TrackNumber;
    if (mode == "ABS_TIME")        return SeekMode::AbsoluteTime;
    if (mode == "REL_TIME")        return SeekMode::RelativeTime;
    if (mode == "ABS_COUNT")       return SeekMode::AbsoluteCount;
    if (mode == "REL_COUNT")       return SeekMode::RelativeCount;
    if (mode == "CHANNEL_FREQ")    return SeekMode::ChannelFrequency;
    if (mode == "TAPE-INDEX")      return SeekMode::TapeIndex;
    if (mode == "FRAME")           return SeekMode::Frame;

    throw std::logic_error("Unknown AVTransport seekmode:" + mode);
}

inline std::string toString(SeekMode mode)
{
    switch (mode)
    {
        case SeekMode::TrackNumber:         return "TRACK_NR";
        case SeekMode::AbsoluteTime:        return "ABS_TIME";
        case SeekMode::RelativeTime:        return "REL_TIME";
        case SeekMode::AbsoluteCount:       return "ABS_COUNT";
        case SeekMode::RelativeCount:       return "REL_COUNT";
        case SeekMode::ChannelFrequency:    return "CHANNEL_FREQ";
        case SeekMode::TapeIndex:           return "TAPE-INDEX";
        case SeekMode::Frame:               return "FRAME";
        default:
            throw std::logic_error("Invalid AVTransport seekmode");
    }
}

inline PlayMode playModeFromString(const std::string& mode)
{
    if (mode == "NORMAL")           return PlayMode::Normal;
    if (mode == "SHUFFLE")          return PlayMode::Shuffle;
    if (mode == "REPEAT_ONE")       return PlayMode::RepeatOne;
    if (mode == "REPEAT_ALL")       return PlayMode::RepeatAll;
    if (mode == "RANDOM")           return PlayMode::Random;
    if (mode == "DIRECT_1")         return PlayMode::Direct;
    if (mode == "INTRO")            return PlayMode::Intro;

    throw std::logic_error("Unknown AVTransport playmode:" + mode);
}

inline std::string toString(PlayMode mode)
{
    switch (mode)
    {
        case PlayMode::Normal:      return "NORMAL";
        case PlayMode::Shuffle:     return "SHUFFLE";
        case PlayMode::RepeatOne:   return "REPEAT_ONE";
        case PlayMode::RepeatAll:   return "REPEAT_ALL";
        case PlayMode::Random:      return "RANDOM";
        case PlayMode::Direct:      return "DIRECT_1";
        case PlayMode::Intro:       return "INTRO";
        default:
            throw std::logic_error("Invalid AVTransport playmode");
    }
}

}
}

#endif
