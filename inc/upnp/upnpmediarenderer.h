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

namespace upnp
{

class Item;
class Device;
class Client;

class MediaRenderer
{
public:
    MediaRenderer(const Client& cp);
    
    void setDevice(std::shared_ptr<Device> device);
    bool supportsPlayback(const Item& item) const;
    
private:
    std::shared_ptr<Device>     m_Device;
    std::vector<ProtocolInfo>   m_ProtocolInfo;
    
    ConnectionManager           m_ConnectionMgr;
    RenderingControl            m_RenderingControl;
};

}

#endif
