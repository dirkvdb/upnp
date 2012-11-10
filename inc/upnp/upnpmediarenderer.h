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

#include "upnp/upnpconnectionmanager.h"
#include "upnp/upnprenderingcontrol.h"
#include "upnp/upnpavtransport.h"

#include "utils/workerthread.h"
#include "utils/signal.h"

namespace upnp
{

class Item;
class Device;
class Client;
class AVTransport;
class ProtocolInfo;
class Resource;
struct ConnectionInfo;

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

    MediaRenderer(IClient& cp);
    MediaRenderer(const MediaRenderer&) = delete;
    
    std::shared_ptr<Device> getDevice();
    void setDevice(const std::shared_ptr<Device>& device);
    bool supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const;
    
    std::string getPeerConnectionId() const;
    
    ConnectionManager& connectionManager();

    // AV Transport
    void setTransportItem(const ConnectionInfo& info, Resource& resource);
    void play(const ConnectionInfo& info);
    void pause(const ConnectionInfo& info);
    void stop(const ConnectionInfo& info);
    void next(const ConnectionInfo& info);
    void previous(const ConnectionInfo& info);
    
    std::string getCurrentTrackURI() const;
    std::string getCurrentTrackDuration() const;
    Item getCurrentTrackInfo() const;
    std::set<Action> getAvailableActions() const;
    bool isActionAvailable(Action action) const;
    
    
    // Rendering control
    void setVolume(const ConnectionInfo& info, uint32_t value);
    uint32_t getVolume();
    
    void activateEvents();
    void deactivateEvents();
    
    utils::Signal<void(uint32_t)>                   VolumeChanged;
    utils::Signal<void()>                           MediaInfoChanged;
    
private:
    void calculateAvailableActions();
    void onRenderingControlLastChangeEvent(const std::map<RenderingControlVariable, std::string>&);
    void onAVTransportLastChangeEvent(const std::map<AVTransportVariable, std::string>& vars);
    
    
    static MediaRenderer::Action transportActionToAction(AVTransportAction action);
    
    std::shared_ptr<Device>                         m_Device;
    std::vector<ProtocolInfo>                       m_ProtocolInfo;
    
    IClient&                                        m_Client;
    ConnectionManager                               m_ConnectionMgr;
    RenderingControl                                m_RenderingControl;
    std::unique_ptr<AVTransport>                    m_AVtransport;
    
    bool                                            m_Active;
    uint32_t                                        m_CurrentVolume;
    
    std::set<Action>                                m_AvailableActions;
    std::map<AVTransportVariable, std::string>      m_AvTransportInfo;
    
    mutable std::mutex                              m_Mutex;
};

}

#endif
