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
class IClient;
class ProtocolInfo;
class Resource;
struct Device;

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

    MediaRenderer(IClient& client);
    MediaRenderer(const MediaRenderer&) = delete;

    std::shared_ptr<Device> getDevice();
    void setDevice(const std::shared_ptr<Device>& device, std::function<void(Status)> cb);
    bool supportsPlayback(const upnp::Item& item, Resource& suggestedResource) const;

    // Connection management
    std::string getPeerConnectionManager() const;
    void resetConnection();
    void useDefaultConnection();
    bool supportsConnectionPreparation() const;
    void prepareConnection(const Resource& resource, const std::string& peerConnectionManager, uint32_t serverConnectionId);

    // AV Transport
    void setTransportItem(const Resource& resource, std::function<void(Status)> cb);
    void setNextTransportItem(const Resource& resource, std::function<void(Status)> cb);
    void play(std::function<void(Status)> cb);
    void pause(std::function<void(Status)> cb);
    void stop(std::function<void(Status)> cb);
    void next(std::function<void(Status)> cb);
    void seekInTrack(std::chrono::seconds position, std::function<void(Status)> cb);
    void previous(std::function<void(Status)> cb);
    void getCurrentTrackPosition(std::function<void(Status, std::chrono::seconds)> cb);
    void getPlaybackState(std::function<void(Status, PlaybackState)> cb);

    std::string getCurrentTrackURI() const;
    std::chrono::seconds getCurrentTrackDuration() const;

    void getCurrentTrackInfo(std::function<void(Status, Item)> cb) const;
    void getAvailableActions(std::function<void(Status, std::set<Action>)> cb);
    static bool isActionAvailable(const std::set<Action>& actions, Action action);

    bool supportsQueueItem() const;


    // Rendering control
    void setVolume(uint32_t value, std::function<void(Status)> cb);
    void getVolume(std::function<void(Status, uint32_t volume)> cb);

    void activateEvents(std::function<void(Status)> cb);
    void deactivateEvents(std::function<void(Status)> cb);

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
    void getProtocolInfo(std::function<void(Status)> cb);

    void onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>&);
    void onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars);

    std::shared_ptr<Device>                         m_device;
    IClient&                                        m_client;
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
