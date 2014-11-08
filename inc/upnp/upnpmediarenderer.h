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

#ifndef UPNP_MEDIA_RENDERER_H
#define UPNP_MEDIA_RENDERER_H

#include <memory>

#include "upnp/upnpconnectionmanagerclient.h"
#include "upnp/upnprenderingcontrolclient.h"
#include "upnp/upnpavtransportclient.h"

#include "utils/workerthread.h"
#include "utils/signal.h"

namespace upnp
{
namespace AVTransport
{
class Client;
}

namespace ConnectionManager
{
struct ConnectionInfo;
}

class Item;
class Device;
class Client;
class ProtocolInfo;
class Resource;

class MediaRenderer
{
public:
    enum class Action
    {
        Play,
        Stop,
        Pause,
        Seek,
        Next,
        Previous,
        Record
    };
    
    enum class PlaybackState
    {
        Stopped,
        Playing,
        Transitioning,
        Paused,
        Recording
    };
    
    MediaRenderer(IClient& cp);
    MediaRenderer(const MediaRenderer&) = delete;
    
    std::shared_ptr<Device> getDevice();
    void setDevice(const std::shared_ptr<Device>& device);
    bool supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const;
    
    // Connection management
    std::string getPeerConnectionManager() const;
    void resetConnection();
    void useDefaultConnection();
    bool supportsConnectionPreparation() const;
    void prepareConnection(const Resource& resource, const std::string& peerConnectionManager, uint32_t serverConnectionId);

    // AV Transport
    void setTransportItem(Resource& resource);
    void setNextTransportItem(Resource& resource);
    void play();
    void pause();
    void stop();
    void next();
    void seekInTrack(uint32_t position);
    void previous();
    uint32_t getCurrentTrackPosition();
    PlaybackState getPlaybackState();
    
    std::string getCurrentTrackURI() const;
    uint32_t getCurrentTrackDuration() const;
    ItemPtr getCurrentTrackInfo() const;
    std::set<Action> getAvailableActions();
    static bool isActionAvailable(const std::set<Action>& actions, Action action);
    
    bool supportsQueueItem() const;
    
    
    // Rendering control
    void setVolume(uint32_t value);
    uint32_t getVolume();
    
    void activateEvents();
    void deactivateEvents();
    
    utils::Signal<std::shared_ptr<Device>>    DeviceChanged;
    utils::Signal<uint32_t>                   VolumeChanged;
    utils::Signal<ItemPtr>                    CurrentTrackChanged;
    utils::Signal<uint32_t>                   CurrentTrackDurationChanged;
    utils::Signal<std::set<Action>>           AvailableActionsChanged;
    utils::Signal<PlaybackState>              PlaybackStateChanged;
    
private:
    void throwOnUnknownConnectionId() const;

    void resetData();
    std::set<Action> parseAvailableActions(const std::string& actions) const ;
    ItemPtr parseCurrentTrack(const std::string& track) const ;
    PlaybackState parsePlaybackState(const std::string& state) const ;
    
    void onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>&);
    void onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars);
    uint32_t parseDuration(const std::string& duration) const;
    std::string positionToString(uint32_t position) const;
    
    static Action transportActionToAction(AVTransport::Action action);
    static PlaybackState transportStateToPlaybackState(AVTransport::State state);
    
    std::shared_ptr<Device>                         m_Device;
    IClient&                                        m_Client;
    ConnectionManager::Client                       m_ConnectionMgr;
    RenderingControl::Client                        m_RenderingControl;
    std::unique_ptr<AVTransport::Client>            m_AVtransport;
    
    std::vector<ProtocolInfo>                       m_ProtocolInfo;
    std::map<AVTransport::Variable, std::string>    m_AvTransportInfo;
    ConnectionManager::ConnectionInfo               m_ConnInfo;
    
    
    bool                                            m_Active;
    
    
    mutable std::mutex                              m_Mutex;
};

}

#endif
