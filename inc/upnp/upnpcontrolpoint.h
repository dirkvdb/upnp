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

#include "utils/signal.h"

#include "upnp/upnpconnectionmanagerclient.h"
#include "upnp/upnpmediarenderer.h"


namespace upnp
{

class Client;
class Item;
class WebServer;
class MediaServer;
class ProtocolInfo;
    
class ControlPoint
{
public:
    ControlPoint(Client& client);
    ControlPoint(const ControlPoint&) = delete;
    
    ControlPoint& operator=(const ControlPoint&) = delete;
    
    void setWebserver(WebServer& webServer);
    void setRendererDevice(const std::shared_ptr<Device>& dev);
    MediaRenderer& getActiveRenderer();
    
    void activate();
    void deactivate();
    
    // these items are immediately set as the current item, send play when stopped to
    // actually start playback
    void playItem(MediaServer& server, const ItemPtr& item);
    void playItems(MediaServer& server, const std::vector<ItemPtr>& items);
    
    // these items are queued for playback after the current item, calling queue multiple times
    // will overwrite the previous queue action, you have to wait until the currently queued item
    // has been proccessed before calling queue again
    void queueItem(MediaServer& server, const ItemPtr& item);
    void queueItems(MediaServer& server, const std::vector<ItemPtr>& items);
    
private:
    void prepareConnection(MediaServer& server, Resource& resource);
    
    void throwOnMissingWebserver();
    void stopPlaybackIfNecessary();
    std::string generatePlaylistFilename();
    std::shared_ptr<Item> createPlaylistItem(const std::string& filename);

    MediaRenderer                       m_Renderer;
    WebServer*                          m_pWebServer;
};
    
}

#endif
