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

DEFINE_UPNP_SERVICE_EXCEPTION(TransitionNotAvailable,               "Transition not available",                     701)
DEFINE_UPNP_SERVICE_EXCEPTION(NoContents,                           "No contents",                                  702)
DEFINE_UPNP_SERVICE_EXCEPTION(ReadError,                            "Read error",                                   703)
DEFINE_UPNP_SERVICE_EXCEPTION(FormatNotSupportedForPlayback,        "Format not supported for playback",            704)
DEFINE_UPNP_SERVICE_EXCEPTION(TransportIsLocked,                    "Transport is locked",                          705)
DEFINE_UPNP_SERVICE_EXCEPTION(WriteError,                           "Write error",                                  706)
DEFINE_UPNP_SERVICE_EXCEPTION(MediaProtectedOrNotWritable,          "Media is protected or not writable",           707)
DEFINE_UPNP_SERVICE_EXCEPTION(FormatNotSupportedForRecording,       "Format not supported for recording",           708)
DEFINE_UPNP_SERVICE_EXCEPTION(MediaIsFull,                          "Media is full",                                709)
DEFINE_UPNP_SERVICE_EXCEPTION(SeekModeNotSupported,                 "Seek mode not supported",                      710)
DEFINE_UPNP_SERVICE_EXCEPTION(IllegalSeekTarget,                    "Illegal seek target",                          711)
DEFINE_UPNP_SERVICE_EXCEPTION(PlayModeNotSupported,                 "Play mode not supported",                      712)
DEFINE_UPNP_SERVICE_EXCEPTION(RecordQualityNotSupported,            "Record quality not supported",                 713)
DEFINE_UPNP_SERVICE_EXCEPTION(IllegalMimeTypeException,             "Illegal MIME type",                            714)
DEFINE_UPNP_SERVICE_EXCEPTION(ContentBusy,                          "Content busy",                                 715)
DEFINE_UPNP_SERVICE_EXCEPTION(ResourceNotFoundException,            "Resource not found",                           716)
DEFINE_UPNP_SERVICE_EXCEPTION(PlaySpeedNotSupported,                "Play speed not supported",                     717)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidInstanceId,                    "Invalid instance id",                          718)
DEFINE_UPNP_SERVICE_EXCEPTION(DRMError,                             "DRM Error",                                    719)
DEFINE_UPNP_SERVICE_EXCEPTION(ExpiredContent,                       "Expired Content",                              720)
DEFINE_UPNP_SERVICE_EXCEPTION(NonAllowedUse,                        "Non-allowed use",                              721)
DEFINE_UPNP_SERVICE_EXCEPTION(CantDetermineAllowedUses,             "Can't determine allowed uses",                 722)
DEFINE_UPNP_SERVICE_EXCEPTION(ExhaustedAllowedUse,                  "Exhausted allowed use",                        723)
DEFINE_UPNP_SERVICE_EXCEPTION(DeviceAuthenticationFailure,          "Device authentication failure",                724)
DEFINE_UPNP_SERVICE_EXCEPTION(DeviceRevocation,                     "Device revocation",                            725)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidStateVariableListException,    "Invalid StateVariableList",                    726)
DEFINE_UPNP_SERVICE_EXCEPTION(IllFormedCSVList,                     "Ill-formed CSV list",                          727)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidStateVariableValue,            "Invalid state variable value",                 728)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidServiceType,                   "Invalid service type",                         729)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidServiceId,                     "Invalid service id",                           730)
DEFINE_UPNP_SERVICE_EXCEPTION(InvalidTimeOffsetPositionValue,       "Invalid time, offset or position value",       731)
DEFINE_UPNP_SERVICE_EXCEPTION(UnableToCalculateSyncPoint,           "Unable to calculate sync point",               732)
DEFINE_UPNP_SERVICE_EXCEPTION(SyncPositionOrOffsetTooEarlyOrSmall,  "Sync, position, or offset too early or small", 733)
DEFINE_UPNP_SERVICE_EXCEPTION(IllegalPlaylistOffsett,               "Illegal playlist offset",                      734)
DEFINE_UPNP_SERVICE_EXCEPTION(IncorrectPLaylistLength,              "Incorrect playlist length",                    735)
DEFINE_UPNP_SERVICE_EXCEPTION(IllegalPlaylist,                      "Illegal playlist",                             736)

enum class Action
{
// AVTransport:1
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
    GetCurrentTransportActions, // Optional

// AVTransport:2
    GetMediaInfoExt,
    GetDRMState, // Optional
    GetStateVariables, // Optional
    SetStateVariables, // Optional

// AVTransport:3
    GetSyncOffset,  // Optional
    AdjustSyncOffset,  // Optional
    SetSyncOffset,  // Optional
    SyncPlay,  // Optional
    SyncStop,  // Optional
    SyncPause,  // Optional
    SetStaticPlaylist,  // Optional
    SetStreamingPlaylist,  // Optional
    GetPlaylistInfo  // Optional
};

enum class Variable
{
// AVTransport:1
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
    CurrentTransportActions, // Optional
    RelativeTimePosition,
    AbsoluteTimePosition,
    RelativeCounterPosition,
    AbsoluteCounterPosition,
    ArgumentTypeSeekMode,
    ArgumentTypeSeekTarget,
    ArgumentTypeInstanceId,
    LastChange, //event

// AVTransport:2
    CurrentMediaCategory,
    DRMState, // Optional
    ArgumentTypeDeviceUDN, // Optional
    ArgumentTypeServiceType, // Optional
    ArgumentTypeServiceId, // Optional
    ArgumentTypeStateVariableValuePairs, // Optional
    ArgumentTypeStateVariableList, // Optional

// AVTransport:3
    SyncOffset, // Optional
    ArgumentTypePlaylistData,
    ArgumentTypePlaylistDataLength,
    ArgumentTypePlaylistOffset,
    ArgumentTypePlaylistTotalLength,
    ArgumentTypePlaylistMIMEType,
    ArgumentTypePlaylistExtendedType,
    ArgumentTypePlaylistStep,
    ArgumentTypePlaylistType,
    ArgumentTypePlaylistInfo,
    ArgumentTypePlaylistStartObjID
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
    RelativeTapeIndex,
    Frame,
    RelativeFrame
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

enum class CurrentMediaCategory
{
    NoMedia,
    TrackAware,
    TrackUnaware
};

enum class DRMState
{
    Ok,
    Unknown,
    ProcessingContentKey,
    ContentKeyFailure,
    AttemptingAuthentication,
    FailedAuthentication,
    NotAuthenticated,
    DeviceRevocation,
    DrmSystemNotSupported,
    LicenseDenied,
    LicenseExpired,
    LicenseInsufficient
};

enum class PlaylistStep
{
    Initial,
    Continue,
    Stop,
    Reset,
    Replace
};

enum class PlaylistType
{
    Static,
    StaticPIContents,
    Streaming
};

enum class PlaylistState
{
    Idle,
    Ready,
    Active,
    Incomplete
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

    if (action == "GetMediaInfoExt")                return Action::GetMediaInfoExt;
    if (action == "GetDRMState")                    return Action::GetDRMState;
    if (action == "GetStateVariables")              return Action::GetStateVariables;
    if (action == "SetStateVariables")              return Action::SetStateVariables;

    if (action == "GetSyncOffset")                  return Action::GetSyncOffset;
    if (action == "AdjustSyncOffset")               return Action::AdjustSyncOffset;
    if (action == "SetSyncOffset")                  return Action::SetSyncOffset;
    if (action == "SyncPlay")                       return Action::SyncPlay;
    if (action == "SyncStop")                       return Action::SyncStop;
    if (action == "SyncPause")                      return Action::SyncPause;
    if (action == "SetStaticPlaylist")              return Action::SetStaticPlaylist;
    if (action == "SetStreamingPlaylist")           return Action::SetStreamingPlaylist;
    if (action == "GetPlaylistInfo")                return Action::GetPlaylistInfo;

    throw Exception("Unknown AVTransport action: {}", action);
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

        case Action::GetMediaInfoExt:               return "GetMediaInfo_Ext";
        case Action::GetDRMState:                   return "GetDRMState";
        case Action::GetStateVariables:             return "GetStateVariables";
        case Action::SetStateVariables:             return "SetStateVariables";

        case Action::GetSyncOffset:                 return "GetSyncOffset";
        case Action::AdjustSyncOffset:              return "AdjustSyncOffset";
        case Action::SetSyncOffset:                 return "SetSyncOffset";
        case Action::SyncPlay:                      return "SyncPlay";
        case Action::SyncStop:                      return "SyncStop";
        case Action::SyncPause:                     return "SyncPause";
        case Action::SetStaticPlaylist:             return "SetStaticPlaylist";
        case Action::SetStreamingPlaylist:          return "SetStreamingPlaylist";
        case Action::GetPlaylistInfo:               return "GetPlaylistInfo";

        default:
            throw Exception("Invalid AVTransport action: {}", static_cast<int32_t>(action));
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "TransportState")                        return Variable::TransportState;
    if (var == "TransportStatus")                       return Variable::TransportStatus;
    if (var == "PlaybackStorageMedium")                 return Variable::PlaybackStorageMedium;
    if (var == "PossiblePlaybackStorageMedia")          return Variable::PossiblePlaybackStorageMedia;
    if (var == "PossibleRecordStorageMedia")            return Variable::PossibleRecordStorageMedia;
    if (var == "CurrentPlayMode")                       return Variable::CurrentPlayMode;
    if (var == "TransportPlaySpeed")                    return Variable::TransportPlaySpeed;
    if (var == "RecordStorageMedium")                   return Variable::RecordStorageMedium;
    if (var == "RecordMediumWriteStatus")               return Variable::RecordMediumWriteStatus;
    if (var == "PossibleRecordQualityModes")            return Variable::PossibleRecordQualityModes;
    if (var == "CurrentRecordQualityMode")              return Variable::CurrentRecordQualityMode;
    if (var == "NumberOfTracks")                        return Variable::NumberOfTracks;
    if (var == "CurrentTrack")                          return Variable::CurrentTrack;
    if (var == "CurrentTrackDuration")                  return Variable::CurrentTrackDuration;
    if (var == "CurrentMediaDuration")                  return Variable::CurrentMediaDuration;
    if (var == "CurrentTrackURI")                       return Variable::CurrentTrackURI;
    if (var == "CurrentTrackMetaData")                  return Variable::CurrentTrackMetaData;
    if (var == "AVTransportURI")                        return Variable::AVTransportURI;
    if (var == "AVTransportURIMetaData")                return Variable::AVTransportURIMetaData;
    if (var == "NextAVTransportURI")                    return Variable::NextAVTransportURI;
    if (var == "NextAVTransportURIMetaData")            return Variable::NextAVTransportURIMetaData;
    if (var == "CurrentTransportActions")               return Variable::CurrentTransportActions;
    if (var == "RelativeTimePosition")                  return Variable::RelativeTimePosition;
    if (var == "AbsoluteTimePosition")                  return Variable::AbsoluteTimePosition;
    if (var == "RelativeCounterPosition")               return Variable::RelativeCounterPosition;
    if (var == "AbsoluteCounterPosition")               return Variable::AbsoluteCounterPosition;
    if (var == "A_ARG_TYPE_SeekMode")                   return Variable::ArgumentTypeSeekMode;
    if (var == "A_ARG_TYPE_SeekTarget")                 return Variable::ArgumentTypeSeekTarget;
    if (var == "A_ARG_TYPE_InstanceID")                 return Variable::ArgumentTypeInstanceId;
    if (var == "LastChange")                            return Variable::LastChange;

    if (var == "CurrentMediaCategory")                  return Variable::CurrentMediaCategory;
    if (var == "DRMState")                              return Variable::DRMState;
    if (var == "A_ARG_TYPE_DeviceUDN")                  return Variable::ArgumentTypeDeviceUDN;
    if (var == "A_ARG_TYPE_ServiceType")                return Variable::ArgumentTypeServiceType;
    if (var == "A_ARG_TYPE_ServiceId")                  return Variable::ArgumentTypeServiceId;
    if (var == "A_ARG_TYPE_StateVariableValuePairs")    return Variable::ArgumentTypeStateVariableValuePairs;
    if (var == "A_ARG_TYPE_StateVariableList")          return Variable::ArgumentTypeStateVariableList;

    if (var == "SyncOffset")                            return Variable::SyncOffset;
    if (var == "A_ARG_TYPE_PlaylistData")               return Variable::ArgumentTypePlaylistData;
    if (var == "A_ARG_TYPE_PlaylistDataLength")         return Variable::ArgumentTypePlaylistDataLength;
    if (var == "A_ARG_TYPE_PlaylistOffset")             return Variable::ArgumentTypePlaylistOffset;
    if (var == "A_ARG_TYPE_PlaylistTotalLength")        return Variable::ArgumentTypePlaylistTotalLength;
    if (var == "A_ARG_TYPE_PlaylistMIMEType")           return Variable::ArgumentTypePlaylistMIMEType;
    if (var == "A_ARG_TYPE_PlaylistExtendedType")       return Variable::ArgumentTypePlaylistExtendedType;
    if (var == "A_ARG_TYPE_PlaylistStep")               return Variable::ArgumentTypePlaylistStep;
    if (var == "A_ARG_TYPE_PlaylistType")               return Variable::ArgumentTypePlaylistType;
    if (var == "A_ARG_TYPE_PlaylistInfo")               return Variable::ArgumentTypePlaylistInfo;
    if (var == "A_ARG_TYPE_PlaylistStartObjID")         return Variable::ArgumentTypePlaylistStartObjID;

    throw Exception("Unknown AVTransport variable: {}", var);
}

inline std::string variableToString(Variable var)
{
    switch (var)
    {
    case Variable::TransportState:                      return "TransportState";
    case Variable::TransportStatus:                     return "TransportStatus";
    case Variable::PlaybackStorageMedium:               return "PlaybackStorageMedium";
    case Variable::PossiblePlaybackStorageMedia:        return "PossiblePlaybackStorageMedia";
    case Variable::PossibleRecordStorageMedia:          return "PossibleRecordStorageMedia";
    case Variable::CurrentPlayMode:                     return "CurrentPlayMode";
    case Variable::TransportPlaySpeed:                  return "TransportPlaySpeed";
    case Variable::RecordStorageMedium:                 return "RecordStorageMedium";
    case Variable::RecordMediumWriteStatus:             return "RecordMediumWriteStatus";
    case Variable::PossibleRecordQualityModes:          return "PossibleRecordQualityModes";
    case Variable::CurrentRecordQualityMode:            return "CurrentRecordQualityMode";
    case Variable::NumberOfTracks:                      return "NumberOfTracks";
    case Variable::CurrentTrack:                        return "CurrentTrack";
    case Variable::CurrentTrackDuration:                return "CurrentTrackDuration";
    case Variable::CurrentMediaDuration:                return "CurrentMediaDuration";
    case Variable::CurrentTrackURI:                     return "CurrentTrackURI";
    case Variable::CurrentTrackMetaData:                return "CurrentTrackMetaData";
    case Variable::AVTransportURI:                      return "AVTransportURI";
    case Variable::AVTransportURIMetaData:              return "AVTransportURIMetaData";
    case Variable::NextAVTransportURI:                  return "NextAVTransportURI";
    case Variable::NextAVTransportURIMetaData:          return "NextAVTransportURIMetaData";
    case Variable::CurrentTransportActions:             return "CurrentTransportActions";
    case Variable::RelativeTimePosition:                return "RelativeTimePosition";
    case Variable::AbsoluteTimePosition:                return "AbsoluteTimePosition";
    case Variable::RelativeCounterPosition:             return "RelativeCounterPosition";
    case Variable::AbsoluteCounterPosition:             return "AbsoluteCounterPosition";
    case Variable::ArgumentTypeSeekMode:                return "A_ARG_TYPE_SeekMode";
    case Variable::ArgumentTypeSeekTarget:              return "A_ARG_TYPE_SeekTarget";
    case Variable::ArgumentTypeInstanceId:              return "A_ARG_TYPE_InstanceID";
    case Variable::LastChange:                          return "LastChange";

    case Variable::CurrentMediaCategory:                return "CurrentMediaCategory";
    case Variable::DRMState:                            return "DRMState";
    case Variable::ArgumentTypeDeviceUDN:               return "A_ARG_TYPE_DeviceUDN";
    case Variable::ArgumentTypeServiceType:             return "A_ARG_TYPE_ServiceType";
    case Variable::ArgumentTypeServiceId:               return "A_ARG_TYPE_ServiceId";
    case Variable::ArgumentTypeStateVariableValuePairs: return "A_ARG_TYPE_StateVariableValuePairs";
    case Variable::ArgumentTypeStateVariableList:       return "A_ARG_TYPE_StateVariableList";

    case Variable::SyncOffset:                          return "SyncOffset";
    case Variable::ArgumentTypePlaylistData:            return "A_ARG_TYPE_PlaylistData";
    case Variable::ArgumentTypePlaylistDataLength:      return "A_ARG_TYPE_PlaylistDataLength";
    case Variable::ArgumentTypePlaylistOffset:          return "A_ARG_TYPE_PlaylistOffset";
    case Variable::ArgumentTypePlaylistTotalLength:     return "A_ARG_TYPE_PlaylistTotalLength";
    case Variable::ArgumentTypePlaylistMIMEType:        return "A_ARG_TYPE_PlaylistMIMEType";
    case Variable::ArgumentTypePlaylistExtendedType:    return "A_ARG_TYPE_PlaylistExtendedType";
    case Variable::ArgumentTypePlaylistStep:            return "A_ARG_TYPE_PlaylistStep";
    case Variable::ArgumentTypePlaylistType:            return "A_ARG_TYPE_PlaylistType";
    case Variable::ArgumentTypePlaylistInfo:            return "A_ARG_TYPE_PlaylistInfo";
    case Variable::ArgumentTypePlaylistStartObjID:      return "A_ARG_TYPE_PlaylistStartObjID";
    default:
        throw Exception("Unknown AVTransport variable: {}", static_cast<int32_t>(var));
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

    throw Exception("Unknown AVTransport state: {}", state);
}

inline PlaylistType playlistTypeFromString(const std::string& type)
{
    if (type == "Static")               return PlaylistType::Static;
    if (type == "StaticPIContents")     return PlaylistType::StaticPIContents;
    if (type == "Streaming")            return PlaylistType::Streaming;

    throw Exception("Unknown playlist type: {}", type);
}

inline PlaylistStep playlistStepFromString(const std::string& step)
{
    if (step == "Initial")              return PlaylistStep::Initial;
    if (step == "Continue")             return PlaylistStep::Continue;
    if (step == "Stop")                 return PlaylistStep::Stop;
    if (step == "Reset")                return PlaylistStep::Reset;
    if (step == "Replace")              return PlaylistStep::Replace;

    throw Exception("Unknown playlist step: {}", step);
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
            throw Exception("Invalid AVTransport state: {}", static_cast<int32_t>(state));
    }
}

inline std::string toString(CurrentMediaCategory state)
{
    switch (state)
    {
        case CurrentMediaCategory::NoMedia:         return "NO_MEDIA";
        case CurrentMediaCategory::TrackAware:      return "TRACK_AWARE";
        case CurrentMediaCategory::TrackUnaware:    return "TRACK_UNAWARE";
        default:
            throw Exception("Invalid media category: {}", static_cast<int32_t>(state));
    }
}

inline std::string toString(DRMState state)
{
    switch (state)
    {
        case DRMState::Ok:                          return "OK";
        case DRMState::Unknown:                     return "UNKNOWN";
        case DRMState::ProcessingContentKey:        return "PROCESSING_CONTENT_KEY";
        case DRMState::ContentKeyFailure:           return "CONTENT_KEY_FAILURE";
        case DRMState::AttemptingAuthentication:    return "ATTEMPTING_AUTHENTICATION";
        case DRMState::FailedAuthentication:        return "FAILED_AUTHENTICATION";
        case DRMState::NotAuthenticated:            return "NOT_AUTHENTICATED";
        case DRMState::DeviceRevocation:            return "DEVICE_REVOCATION";
        case DRMState::DrmSystemNotSupported:       return "DRM_SYSTEM_NOT_SUPPORTED";
        case DRMState::LicenseDenied:               return "LICENSE_DENIED";
        case DRMState::LicenseExpired:              return "LICENSE_EXPIRED";
        case DRMState::LicenseInsufficient:         return "LICENSE_INSUFFICIENT";
        default:
            throw Exception("Invalid DRM state: {}", static_cast<int32_t>(state));
    }
}

inline std::string toString(PlaylistStep step)
{
    switch (step)
    {
    case PlaylistStep::Initial:     return "Initial";
    case PlaylistStep::Continue:    return "Continue";
    case PlaylistStep::Stop:        return "Stop";
    case PlaylistStep::Reset:       return "Reset";
    case PlaylistStep::Replace:     return "Replace";
    default:
        throw Exception("Invalid playlist step: {}", static_cast<int32_t>(step));
    }
}

inline std::string toString(PlaylistType type)
{
    switch (type)
    {
    case PlaylistType::Static:              return "Static";
    case PlaylistType::StaticPIContents:    return "StaticPIContents";
    case PlaylistType::Streaming:           return "Streaming";
    default:
        throw Exception("Invalid playlist type: {}", static_cast<int32_t>(type));
    }
}

inline std::string toString(PlaylistState state)
{
    switch (state)
    {
    case PlaylistState::Idle:       return "Idle";
    case PlaylistState::Ready:      return "Ready";
    case PlaylistState::Active:     return "Active";
    case PlaylistState::Incomplete: return "Incomplete";
    default:
        throw Exception("Invalid playlist state: {}", static_cast<int32_t>(state));
    }
}

inline Status statusFromString(const std::string& status)
{
    if (status == "OK")                 return Status::Ok;
    if (status == "ERROR_OCCURRED")     return Status::Error;

    throw Exception("Unknown AVTransport status: {}", status);
}

inline std::string toString(Status status)
{
    switch (status)
    {
        case Status::Ok:            return "OK";
        case Status::Error:         return "ERROR_OCCURRED";
        default:
            throw Exception("Invalid AVTransport status: {}", static_cast<int32_t>(status));
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
    if (mode == "REL_TAPE-INDEX")  return SeekMode::RelativeTapeIndex;
    if (mode == "FRAME")           return SeekMode::Frame;
    if (mode == "REL_FRAME")       return SeekMode::RelativeFrame;

    throw Exception("Unknown AVTransport seekmode: {}", mode);
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
        case SeekMode::RelativeTapeIndex:   return "REL_TAPE-INDEX";
        case SeekMode::Frame:               return "FRAME";
        case SeekMode::RelativeFrame:       return "REL_FRAME";
        default:
            throw Exception("Invalid AVTransport seekmode: {}", static_cast<int32_t>(mode));
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

    throw Exception("Unknown AVTransport playmode: {}", mode);
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
            throw Exception("Invalid AVTransport playmode: {}", static_cast<int32_t>(mode));
    }
}

}
}

#endif
