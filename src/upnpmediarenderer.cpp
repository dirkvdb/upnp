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

#include "upnp/upnpmediarenderer.h"

#include "upnp/upnpitem.h"

namespace upnp
{

MediaRenderer::MediaRenderer(const Client& client)
: m_ConnectionMgr(client)
, m_RenderingControl(client)
{
}

void MediaRenderer::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
    m_ConnectionMgr.setDevice(device);
    m_RenderingControl.setDevice(device);
    
    m_ProtocolInfo = m_ConnectionMgr.getProtocolInfo();
}

bool MediaRenderer::supportsPlayback(const Item& item) const
{
    for (auto& res : item.getResources())
    {
        for (auto& info : m_ProtocolInfo)
        {
            if (info.isCompatibleWith(res.getProtocolInfo()))
            {
                return true;
            }
        }
    }
    
    return false;
}

}
