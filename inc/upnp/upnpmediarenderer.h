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

    MediaRenderer(Client& cp);
    MediaRenderer(const MediaRenderer&) = delete;
    
    std::shared_ptr<Device> getDevice();
    void setDevice(const std::shared_ptr<Device>& device);
    bool supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const;
    
    std::string getPeerConnectionId() const;
    
    ConnectionManager& connectionManager();

    void setTransportItem(const ConnectionInfo& info, Resource& resource);
    void play(const ConnectionInfo& info);
    void stop(const ConnectionInfo& info);
    
    void activateEvents();
    void deactivateEvents();
    
    bool isActionAvailable(Action action) const;
    
    utils::Signal<void(const std::set<Action>&)>    AvailableActionsChanged;
    
private:
    void onLastChanged(const std::map<AVTransport::Variable, std::string>& vars);
    void updateAvailableActions(const std::string& actionList);
    
    static MediaRenderer::Action transportActionToAction(AVTransport::Action action);
    
    std::shared_ptr<Device>             m_Device;
    std::vector<ProtocolInfo>           m_ProtocolInfo;
    std::set<Action>                    m_AvailableActions;
    
    Client&                             m_Client;
    ConnectionManager                   m_ConnectionMgr;
    RenderingControl                    m_RenderingControl;
    std::unique_ptr<AVTransport>        m_AVtransport;
};

}

#endif
