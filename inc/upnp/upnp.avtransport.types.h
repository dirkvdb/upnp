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

#pragma once

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
    GetPlaylistInfo,  // Optional

    EnumCount
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
    ArgumentTypePlaylistStartObjID,

    EnumCount
};

enum class State
{
    Stopped,
    Playing,
    Transitioning,
    PausedPlayback,
    PausedRecording,
    Recording,
    NoMediaPresent,

    EnumCount
};

enum class Status
{
    Ok,
    Error,

    EnumCount
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
    RelativeFrame,

    EnumCount
};

enum class PlayMode
{
    Normal,
    Shuffle,
    RepeatOne,
    RepeatAll,
    Random,
    Direct,
    Intro,

    EnumCount
};

enum class CurrentMediaCategory
{
    NoMedia,
    TrackAware,
    TrackUnaware,

    EnumCount
};

enum class DRMState
{
    Ok,
    DRMUnknown,
    ProcessingContentKey,
    ContentKeyFailure,
    AttemptingAuthentication,
    FailedAuthentication,
    NotAuthenticated,
    DeviceRevocation,
    DrmSystemNotSupported,
    LicenseDenied,
    LicenseExpired,
    LicenseInsufficient,

    EnumCount
};

enum class PlaylistStep
{
    Initial,
    Continue,
    Stop,
    Reset,
    Replace,

    EnumCount
};

enum class PlaylistType
{
    Static,
    StaticPIContents,
    Streaming,

    EnumCount
};

enum class PlaylistState
{
    Idle,
    Ready,
    Active,
    Incomplete,

    EnumCount
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

}
}
