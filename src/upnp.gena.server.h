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

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

#include "upnp/upnp.uv.h"
#include "upnp/upnp.types.h"
#include "upnp/upnp.http.server.h"

namespace upnp
{
namespace gena
{

// TODO: handle message with chunked encoding
class Server
{
public:
    Server(uv::Loop& loop, const uv::Address& address, std::function<void(const SubscriptionEvent&)> cb);

    void stop(std::function<void()> cb);
    uv::Address getAddress() const;

private:
    http::Server m_httpServer;
    std::function<void(const SubscriptionEvent&)> m_eventCb;
    SubscriptionEvent m_currentEvent;
};

}
}