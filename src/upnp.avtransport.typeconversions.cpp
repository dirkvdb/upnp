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


#include "upnp.avtransport.typeconversions.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace AVTransport;

static constexpr EnumMap<Action> s_actionNames {{
    std::make_tuple("SetAVTransportURI",              Action::SetAVTransportURI),
    std::make_tuple("SetNextAVTransportURI",          Action::SetNextAVTransportURI),
    std::make_tuple("GetMediaInfo",                   Action::GetMediaInfo),
    std::make_tuple("GetTransportInfo",               Action::GetTransportInfo),
    std::make_tuple("GetPositionInfo",                Action::GetPositionInfo),
    std::make_tuple("GetDeviceCapabilities",          Action::GetDeviceCapabilities),
    std::make_tuple("GetTransportSettings",           Action::GetTransportSettings),
    std::make_tuple("Stop",                           Action::Stop),
    std::make_tuple("Play",                           Action::Play),
    std::make_tuple("Pause",                          Action::Pause),
    std::make_tuple("Record",                         Action::Record),
    std::make_tuple("Seek",                           Action::Seek),
    std::make_tuple("Next",                           Action::Next),
    std::make_tuple("Previous",                       Action::Previous),
    std::make_tuple("SetPlayMode",                    Action::SetPlayMode),
    std::make_tuple("SetRecordQualityMode",           Action::SetRecordQualityMode),
    std::make_tuple("GetCurrentTransportActions",     Action::GetCurrentTransportActions),

    std::make_tuple("GetMediaInfoExt",                Action::GetMediaInfoExt),
    std::make_tuple("GetDRMState",                    Action::GetDRMState),
    std::make_tuple("GetStateVariables",              Action::GetStateVariables),
    std::make_tuple("SetStateVariables",              Action::SetStateVariables),

    std::make_tuple("GetSyncOffset",                  Action::GetSyncOffset),
    std::make_tuple("AdjustSyncOffset",               Action::AdjustSyncOffset),
    std::make_tuple("SetSyncOffset",                  Action::SetSyncOffset),
    std::make_tuple("SyncPlay",                       Action::SyncPlay),
    std::make_tuple("SyncStop",                       Action::SyncStop),
    std::make_tuple("SyncPause",                      Action::SyncPause),
    std::make_tuple("SetStaticPlaylist",              Action::SetStaticPlaylist),
    std::make_tuple("SetStreamingPlaylist",           Action::SetStreamingPlaylist),
    std::make_tuple("GetPlaylistInfo",                Action::GetPlaylistInfo),
}};

static constexpr EnumMap<Variable> s_variableNames {{
    std::make_tuple("TransportState",                        Variable::TransportState),
    std::make_tuple("TransportStatus",                       Variable::TransportStatus),
    std::make_tuple("PlaybackStorageMedium",                 Variable::PlaybackStorageMedium),
    std::make_tuple("PossiblePlaybackStorageMedia",          Variable::PossiblePlaybackStorageMedia),
    std::make_tuple("PossibleRecordStorageMedia",            Variable::PossibleRecordStorageMedia),
    std::make_tuple("CurrentPlayMode",                       Variable::CurrentPlayMode),
    std::make_tuple("TransportPlaySpeed",                    Variable::TransportPlaySpeed),
    std::make_tuple("RecordStorageMedium",                   Variable::RecordStorageMedium),
    std::make_tuple("RecordMediumWriteStatus",               Variable::RecordMediumWriteStatus),
    std::make_tuple("PossibleRecordQualityModes",            Variable::PossibleRecordQualityModes),
    std::make_tuple("CurrentRecordQualityMode",              Variable::CurrentRecordQualityMode),
    std::make_tuple("NumberOfTracks",                        Variable::NumberOfTracks),
    std::make_tuple("CurrentTrack",                          Variable::CurrentTrack),
    std::make_tuple("CurrentTrackDuration",                  Variable::CurrentTrackDuration),
    std::make_tuple("CurrentMediaDuration",                  Variable::CurrentMediaDuration),
    std::make_tuple("CurrentTrackURI",                       Variable::CurrentTrackURI),
    std::make_tuple("CurrentTrackMetaData",                  Variable::CurrentTrackMetaData),
    std::make_tuple("AVTransportURI",                        Variable::AVTransportURI),
    std::make_tuple("AVTransportURIMetaData",                Variable::AVTransportURIMetaData),
    std::make_tuple("NextAVTransportURI",                    Variable::NextAVTransportURI),
    std::make_tuple("NextAVTransportURIMetaData",            Variable::NextAVTransportURIMetaData),
    std::make_tuple("CurrentTransportActions",               Variable::CurrentTransportActions),
    std::make_tuple("RelativeTimePosition",                  Variable::RelativeTimePosition),
    std::make_tuple("AbsoluteTimePosition",                  Variable::AbsoluteTimePosition),
    std::make_tuple("RelativeCounterPosition",               Variable::RelativeCounterPosition),
    std::make_tuple("AbsoluteCounterPosition",               Variable::AbsoluteCounterPosition),
    std::make_tuple("A_ARG_TYPE_SeekMode",                   Variable::ArgumentTypeSeekMode),
    std::make_tuple("A_ARG_TYPE_SeekTarget",                 Variable::ArgumentTypeSeekTarget),
    std::make_tuple("A_ARG_TYPE_InstanceID",                 Variable::ArgumentTypeInstanceId),
    std::make_tuple("LastChange",                            Variable::LastChange),

    std::make_tuple("CurrentMediaCategory",                  Variable::CurrentMediaCategory),
    std::make_tuple("DRMState",                              Variable::DRMState),
    std::make_tuple("A_ARG_TYPE_DeviceUDN",                  Variable::ArgumentTypeDeviceUDN),
    std::make_tuple("A_ARG_TYPE_ServiceType",                Variable::ArgumentTypeServiceType),
    std::make_tuple("A_ARG_TYPE_ServiceId",                  Variable::ArgumentTypeServiceId),
    std::make_tuple("A_ARG_TYPE_StateVariableValuePairs",    Variable::ArgumentTypeStateVariableValuePairs),
    std::make_tuple("A_ARG_TYPE_StateVariableList",          Variable::ArgumentTypeStateVariableList),

    std::make_tuple("SyncOffset",                            Variable::SyncOffset),
    std::make_tuple("A_ARG_TYPE_PlaylistData",               Variable::ArgumentTypePlaylistData),
    std::make_tuple("A_ARG_TYPE_PlaylistDataLength",         Variable::ArgumentTypePlaylistDataLength),
    std::make_tuple("A_ARG_TYPE_PlaylistOffset",             Variable::ArgumentTypePlaylistOffset),
    std::make_tuple("A_ARG_TYPE_PlaylistTotalLength",        Variable::ArgumentTypePlaylistTotalLength),
    std::make_tuple("A_ARG_TYPE_PlaylistMIMEType",           Variable::ArgumentTypePlaylistMIMEType),
    std::make_tuple("A_ARG_TYPE_PlaylistExtendedType",       Variable::ArgumentTypePlaylistExtendedType),
    std::make_tuple("A_ARG_TYPE_PlaylistStep",               Variable::ArgumentTypePlaylistStep),
    std::make_tuple("A_ARG_TYPE_PlaylistType",               Variable::ArgumentTypePlaylistType),
    std::make_tuple("A_ARG_TYPE_PlaylistInfo",               Variable::ArgumentTypePlaylistInfo),
    std::make_tuple("A_ARG_TYPE_PlaylistStartObjID",         Variable::ArgumentTypePlaylistStartObjID),
}};

static constexpr EnumMap<State> s_stateNames {{
    std::make_tuple("STOPPED",             State::Stopped),
    std::make_tuple("PLAYING",             State::Playing),
    std::make_tuple("TRANSITIONING",       State::Transitioning),
    std::make_tuple("PAUSED_PLAYBACK",     State::PausedPlayback),
    std::make_tuple("PAUSED_RECORDING",    State::PausedRecording),
    std::make_tuple("RECORDING",           State::Recording),
    std::make_tuple("NO_MEDIA_PRESENT",    State::NoMediaPresent),
}};

static constexpr EnumMap<PlaylistType> s_playlistTypeNames {{
    std::make_tuple("Static",               PlaylistType::Static),
    std::make_tuple("StaticPIContents",     PlaylistType::StaticPIContents),
    std::make_tuple("Streaming",            PlaylistType::Streaming),
}};

static constexpr EnumMap<PlaylistState> s_playlistStateNames {{
    std::make_tuple("Idle",           PlaylistState::Idle),
    std::make_tuple("Ready",          PlaylistState::Ready),
    std::make_tuple("Active",         PlaylistState::Active),
    std::make_tuple("Incomplete",     PlaylistState::Incomplete),
}};

static constexpr EnumMap<PlaylistStep> s_playlistStepNames {{
    std::make_tuple("Initial",                    PlaylistStep::Initial),
    std::make_tuple("Continue",                   PlaylistStep::Continue),
    std::make_tuple("Stop",                       PlaylistStep::Stop),
    std::make_tuple("Reset",                      PlaylistStep::Reset),
    std::make_tuple("Replace",                    PlaylistStep::Replace),
}};

static constexpr EnumMap<CurrentMediaCategory> s_curMediaCategoryNames {{
    std::make_tuple("NO_MEDIA",                   CurrentMediaCategory::NoMedia),
    std::make_tuple("TRACK_AWARE",                CurrentMediaCategory::TrackAware),
    std::make_tuple("TRACK_UNAWARE",              CurrentMediaCategory::TrackUnaware),
}};

static constexpr EnumMap<DRMState> s_drmStateNames {{
    std::make_tuple("OK",                         DRMState::Ok),
    std::make_tuple("UNKNOWN",                    DRMState::DRMUnknown),
    std::make_tuple("PROCESSING_CONTENT_KEY",     DRMState::ProcessingContentKey),
    std::make_tuple("CONTENT_KEY_FAILURE",        DRMState::ContentKeyFailure),
    std::make_tuple("ATTEMPTING_AUTHENTICATION",  DRMState::AttemptingAuthentication),
    std::make_tuple("FAILED_AUTHENTICATION",      DRMState::FailedAuthentication),
    std::make_tuple("NOT_AUTHENTICATED",          DRMState::NotAuthenticated),
    std::make_tuple("DEVICE_REVOCATION",          DRMState::DeviceRevocation),
    std::make_tuple("DRM_SYSTEM_NOT_SUPPORTED",   DRMState::DrmSystemNotSupported),
    std::make_tuple("LICENSE_DENIED",             DRMState::LicenseDenied),
    std::make_tuple("LICENSE_EXPIRED",            DRMState::LicenseExpired),
    std::make_tuple("LICENSE_INSUFFICIENT",       DRMState::LicenseInsufficient),
}};

static constexpr EnumMap<AVTransport::Status> s_statusNames {{
    std::make_tuple("OK",                         AVTransport::Status::Ok),
    std::make_tuple("ERROR_OCCURRED",             AVTransport::Status::Error),
}};

static constexpr EnumMap<PlayMode> s_playModeNames {{
    std::make_tuple("NORMAL",           PlayMode::Normal),
    std::make_tuple("SHUFFLE",          PlayMode::Shuffle),
    std::make_tuple("REPEAT_ONE",       PlayMode::RepeatOne),
    std::make_tuple("REPEAT_ALL",       PlayMode::RepeatAll),
    std::make_tuple("RANDOM",           PlayMode::Random),
    std::make_tuple("DIRECT_1",         PlayMode::Direct),
    std::make_tuple("INTRO",            PlayMode::Intro),
}};

static constexpr EnumMap<SeekMode> s_seekModeNames {{
    std::make_tuple("TRACK_NR",        SeekMode::TrackNumber),
    std::make_tuple("ABS_TIME",        SeekMode::AbsoluteTime),
    std::make_tuple("REL_TIME",        SeekMode::RelativeTime),
    std::make_tuple("ABS_COUNT",       SeekMode::AbsoluteCount),
    std::make_tuple("REL_COUNT",       SeekMode::RelativeCount),
    std::make_tuple("CHANNEL_FREQ",    SeekMode::ChannelFrequency),
    std::make_tuple("TAPE-INDEX",      SeekMode::TapeIndex),
    std::make_tuple("REL_TAPE-INDEX",  SeekMode::RelativeTapeIndex),
    std::make_tuple("FRAME",           SeekMode::Frame),
    std::make_tuple("REL_FRAME",       SeekMode::RelativeFrame),
}};

ADD_ENUM_MAP(Action, s_actionNames)
ADD_ENUM_MAP(Variable, s_variableNames)
ADD_ENUM_MAP(State, s_stateNames)
ADD_ENUM_MAP(PlaylistType, s_playlistTypeNames)
ADD_ENUM_MAP(PlaylistState, s_playlistStateNames)
ADD_ENUM_MAP(PlaylistStep, s_playlistStepNames)
ADD_ENUM_MAP(CurrentMediaCategory, s_curMediaCategoryNames)
ADD_ENUM_MAP(DRMState, s_drmStateNames)
ADD_ENUM_MAP(AVTransport::Status, s_statusNames)
ADD_ENUM_MAP(PlayMode, s_playModeNames)
ADD_ENUM_MAP(SeekMode, s_seekModeNames)

Action AVTransport::actionFromString(gsl::cstring_span<> value)
{
    return enum_cast<Action>(value);
}

const char* AVTransport::actionToString(Action value)
{
    return enum_string(value);
}

Variable AVTransport::variableFromString(gsl::cstring_span<> value)
{
    return enum_cast<Variable>(value);
}

const char* AVTransport::variableToString(Variable value)
{
    return enum_string(value);
}

State AVTransport::stateFromString(gsl::cstring_span<> value)
{
    return enum_cast<State>(value);
}

PlaylistType AVTransport::playlistTypeFromString(gsl::cstring_span<> value)
{
    return enum_cast<PlaylistType>(value);
}

PlaylistStep AVTransport::playlistStepFromString(gsl::cstring_span<> value)
{
    return enum_cast<PlaylistStep>(value);
}

const char* AVTransport::toString(State value)
{
    return enum_string(value);
}

const char* AVTransport::toString(CurrentMediaCategory value)
{
    return enum_string(value);
}

const char* AVTransport::toString(DRMState value)
{
    return enum_string(value);
}

const char* AVTransport::toString(PlaylistStep value)
{
    return enum_string(value);
}

const char* AVTransport::toString(PlaylistType value)
{
    return enum_string(value);
}

const char* AVTransport::toString(PlaylistState value)
{
    return enum_string(value);
}

AVTransport::Status AVTransport::statusFromString(gsl::cstring_span<> value)
{
    return enum_cast<AVTransport::Status>(value);
}

const char* AVTransport::toString(AVTransport::Status value)
{
    return enum_string(value);
}

SeekMode AVTransport::seekModeFromString(gsl::cstring_span<> value)
{
    return enum_cast<SeekMode>(value);
}

const char* AVTransport::toString(SeekMode value)
{
    return enum_string(value);
}

PlayMode AVTransport::playModeFromString(gsl::cstring_span<> value)
{
    return enum_cast<PlayMode>(value);
}

const char* AVTransport::toString(PlayMode value)
{
    return enum_string(value);
}

}
