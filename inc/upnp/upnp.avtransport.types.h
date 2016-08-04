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

#include "upnp/upnp.servicefaults.h"

namespace upnp
{
namespace AVTransport
{

DEFINE_UPNP_SERVICE_FAULT(TransitionNotAvailable,               701, "Transition not available")
DEFINE_UPNP_SERVICE_FAULT(NoContents,                           702, "No contents")
DEFINE_UPNP_SERVICE_FAULT(ReadError,                            703, "Read error")
DEFINE_UPNP_SERVICE_FAULT(FormatNotSupportedForPlayback,        704, "Format not supported for playback")
DEFINE_UPNP_SERVICE_FAULT(TransportIsLocked,                    705, "Transport is locked")
DEFINE_UPNP_SERVICE_FAULT(WriteError,                           706, "Write error")
DEFINE_UPNP_SERVICE_FAULT(MediaProtectedOrNotWritable,          707, "Media is protected or not writable")
DEFINE_UPNP_SERVICE_FAULT(FormatNotSupportedForRecording,       708, "Format not supported for recording")
DEFINE_UPNP_SERVICE_FAULT(MediaIsFull,                          709, "Media is full")
DEFINE_UPNP_SERVICE_FAULT(SeekModeNotSupported,                 710, "Seek mode not supported")
DEFINE_UPNP_SERVICE_FAULT(IllegalSeekTarget,                    711, "Illegal seek target")
DEFINE_UPNP_SERVICE_FAULT(PlayModeNotSupported,                 712, "Play mode not supported")
DEFINE_UPNP_SERVICE_FAULT(RecordQualityNotSupported,            713, "Record quality not supported")
DEFINE_UPNP_SERVICE_FAULT(IllegalMimeTypeException,             714, "Illegal MIME type")
DEFINE_UPNP_SERVICE_FAULT(ContentBusy,                          715, "Content busy")
DEFINE_UPNP_SERVICE_FAULT(ResourceNotFoundException,            716, "Resource not found")
DEFINE_UPNP_SERVICE_FAULT(PlaySpeedNotSupported,                717, "Play speed not supported")
DEFINE_UPNP_SERVICE_FAULT(InvalidInstanceId,                    718, "Invalid instance id")
DEFINE_UPNP_SERVICE_FAULT(DRMError,                             719, "DRM Error")
DEFINE_UPNP_SERVICE_FAULT(ExpiredContent,                       720, "Expired Content")
DEFINE_UPNP_SERVICE_FAULT(NonAllowedUse,                        721, "Non-allowed use")
DEFINE_UPNP_SERVICE_FAULT(CantDetermineAllowedUses,             722, "Can't determine allowed uses")
DEFINE_UPNP_SERVICE_FAULT(ExhaustedAllowedUse,                  723, "Exhausted allowed use")
DEFINE_UPNP_SERVICE_FAULT(DeviceAuthenticationFailure,          724, "Device authentication failure")
DEFINE_UPNP_SERVICE_FAULT(DeviceRevocation,                     725, "Device revocation")
DEFINE_UPNP_SERVICE_FAULT(InvalidStateVariableListException,    726, "Invalid StateVariableList")
DEFINE_UPNP_SERVICE_FAULT(IllFormedCSVList,                     727, "Ill-formed CSV list")
DEFINE_UPNP_SERVICE_FAULT(InvalidStateVariableValue,            728, "Invalid state variable value")
DEFINE_UPNP_SERVICE_FAULT(InvalidServiceType,                   729, "Invalid service type")
DEFINE_UPNP_SERVICE_FAULT(InvalidServiceId,                     730, "Invalid service id")
DEFINE_UPNP_SERVICE_FAULT(InvalidTimeOffsetPositionValue,       731, "Invalid time, offset or position value")
DEFINE_UPNP_SERVICE_FAULT(UnableToCalculateSyncPoint,           732, "Unable to calculate sync point")
DEFINE_UPNP_SERVICE_FAULT(SyncPositionOrOffsetTooEarlyOrSmall,  733, "Sync, position, or offset too early or small")
DEFINE_UPNP_SERVICE_FAULT(IllegalPlaylistOffsett,               734, "Illegal playlist offset")
DEFINE_UPNP_SERVICE_FAULT(IncorrectPLaylistLength,              735, "Incorrect playlist length")
DEFINE_UPNP_SERVICE_FAULT(IllegalPlaylist,                      736, "Illegal playlist")

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
