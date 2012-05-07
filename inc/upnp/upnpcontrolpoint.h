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

#ifndef UPNP_CONTROL_POINT_H
#define UPNP_CONTROL_POINT_H

#include <string>
#include <mutex>
#include <upnp/upnp.h>

#include "upnp/upnpconnectionmanager.h"
#include "upnp/upnpmediarenderer.h"

namespace upnp
{

class Client;
class Item;
class MediaServer;
class ProtocolInfo;
    
class ControlPoint
{
public:
    ControlPoint(Client& client);
    ControlPoint(const ControlPoint&) = delete;
    ~ControlPoint();
    
    void setRendererDevice(std::shared_ptr<Device> dev);
    std::shared_ptr<Device> getActiveRenderer();
    
    ControlPoint& operator=(const ControlPoint&) = delete;
    
    void playItem(MediaServer& server, Item& item);
    void stop();
    
private:
    Client&                             m_Client;
    MediaRenderer                       m_Renderer;
    ConnectionInfo                      m_ConnInfo;
    
    bool                                m_RendererSupportsPrepareForConnection;
};
    
}

#endif
