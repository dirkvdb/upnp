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

#ifndef UPNP_AV_TRANSPORT_H
#define UPNP_AV_TRANSPORT_H

#include "utils/signal.h"
#include "upnp/upnpservicebase.h"

namespace upnp
{
    
class Item;
class Action;

enum class AVTransportAction
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

enum class AVTransportVariable
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

class AVTransport : public ServiceBase<AVTransportAction, AVTransportVariable>
{
public:
    enum class TransportState
    {
        Stopped,
        Playing,
        Transitioning,
        PausedPlayback,
        PausedRecording,
        Recording,
        NoMediaPresent
    };
    
    struct PositionInfo
    {
        std::string track;
        std::string trackDuration;
        std::string trackMetaData;
        std::string trackURI;
        std::string relTime;
        std::string absTime;
        std::string relCount;
        std::string absCount;
    };
    
    struct TransportInfo
    {
        std::string currentTransportState;
        std::string currentTransportStatus;
        std::string currentSpeed;
    };

    AVTransport(IClient& client);
    
    void setAVTransportURI(const std::string& connectionId, const std::string& uri, const std::string& uriMetaData = "");
    void play(const std::string& connectionId, const std::string& speed = "1");
    void pause(const std::string& connectionId);
    void stop(const std::string& connectionId);
    void previous(const std::string& connectionId);
    void next(const std::string& connectionId);
    PositionInfo getPositionInfo(const std::string& connectionId);
    TransportInfo getTransportInfo(const std::string& connectionId);
    
    AVTransportAction actionFromString(const std::string& action);
    std::string actionToString(AVTransportAction action);
    AVTransportVariable variableFromString(const std::string& action);
    std::string variableToString(AVTransportVariable var);
    
    utils::Signal<void(const std::map<AVTransportVariable, std::string>&)> LastChangeEvent;
    
protected:
    void handleStateVariableEvent(AVTransportVariable var, const std::map<AVTransportVariable, std::string>& variables);

    ServiceType getType();
    int32_t getSubscriptionTimeout();
    
    void handleUPnPResult(int errorCode);
};
    
}

#endif
