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

#include "upnp/upnp.connectionmanager.client.h"
#include "upnp/upnp.renderingcontrol.client.h"
#include "upnp/upnp.avtransport.client.h"

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
class IClient2;
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

    MediaRenderer(IClient2& client);
    MediaRenderer(const MediaRenderer&) = delete;

    std::shared_ptr<Device> getDevice();
    void setDevice(const std::shared_ptr<Device>& device, std::function<void(int32_t)> cb);
    bool supportsPlayback(const upnp::Item& item, Resource& suggestedResource) const;

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
    void getCurrentTrackPosition(std::function<void(int32_t, std::chrono::seconds)> cb);
    void getPlaybackState(std::function<void(int32_t, PlaybackState)> cb);

    std::string getCurrentTrackURI() const;
    std::chrono::seconds getCurrentTrackDuration() const;

    void getCurrentTrackInfo(std::function<void(int32_t, Item)> cb) const;
    void getAvailableActions(std::function<void(int32_t, std::set<Action>)> cb);
    static bool isActionAvailable(const std::set<Action>& actions, Action action);

    bool supportsQueueItem() const;


    // Rendering control
    void setVolume(uint32_t value);
    void getVolume(std::function<void(int32_t status, uint32_t volume)> cb);

    void activateEvents();
    void deactivateEvents();

    utils::Signal<std::shared_ptr<Device>>    DeviceChanged;
    utils::Signal<uint32_t>                   VolumeChanged;
    utils::Signal<const Item&>                CurrentTrackChanged;
    utils::Signal<std::chrono::seconds>       CurrentTrackDurationChanged;
    utils::Signal<std::set<Action>>           AvailableActionsChanged;
    utils::Signal<PlaybackState>              PlaybackStateChanged;

private:
    void throwOnUnknownConnectionId() const;

    void resetData();
    std::set<Action> parseAvailableActions(const std::string& actions) const;
    void getProtocolInfo(std::function<void(int32_t)> cb);

    void onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>&);
    void onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars);
    std::string positionToString(uint32_t position) const;

    std::shared_ptr<Device>                         m_device;
    IClient2&                                       m_client;
    ConnectionManager::Client                       m_connectionMgr;
    RenderingControl::Client                        m_renderingControl;
    std::unique_ptr<AVTransport::Client>            m_avTransport;

    std::vector<ProtocolInfo>                       m_protocolInfo;
    std::map<AVTransport::Variable, std::string>    m_avTransportInfo;
    ConnectionManager::ConnectionInfo               m_connInfo;

    bool                                            m_active;
    mutable std::mutex                              m_mutex;
};

}

#endif
