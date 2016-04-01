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

#include "upnp.client.h"

#include "utils/log.h"
#include "upnp/upnp.uv.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.client.h"

#include <stdexcept>
#include <algorithm>
#include <cstring>

using namespace utils;

//#define DEBUG_UPNP_CLIENT

namespace upnp
{

Client2::Client2(uv::Loop& loop)
: m_loop(loop)
{
}

void Client2::initialize(const std::string& interfaceName, int32_t port)
{
    log::debug("Initializing UPnP SDK");
    for (auto& intf : uv::getNetworkInterfaces())
    {
        if (intf.name == interfaceName)
        {
        }
    }

    throw std::runtime_error("Could not find network interface with name: " + interfaceName);
}

void Client2::uninitialize()
{
    log::debug("Uninitialized UPnP SDK");
}

std::string Client2::getIpAddress() const
{
    return "";
}

int32_t Client2::getPort() const
{
    return 0;
}

void Client2::subscribeToService(const std::string& publisherUrl, int32_t /*timeout*/, std::function<void(int32_t status, std::string subId)> /*cb*/) const
{
    log::debug("Subscribe to service: {}", publisherUrl);

    //Upnp_SID subscriptionId;
    //handleUPnPResult(UpnpSubscribe(*m_client, publisherUrl.c_str(), &timeout, subscriptionId), "Failed to subscribe to UPnP device service");
}

void Client2::unsubscribeFromService(const std::string& /*subscriptionId*/) const
{
}

// void Client2::subscribeToService(const std::string& publisherUrl, int32_t /*timeout*/, IServiceSubscriber& /*sub*/) const
// {
//     log::debug("Subscribe to service: {}", publisherUrl);
//     //handleUPnPResult(UpnpSubscribeAsync(*m_client, publisherUrl.c_str(), timeout, &Client2::upnpServiceCallback, sub.get()), "Failed to subscribe to UPnP device service");
//     //m_serviceSubscriptions.insert(std::make_pair(sub.get(), std::weak_ptr<IServiceSubscriber>(sub)));
// }

// void Client2::unsubscribeFromService(IServiceSubscriber& /*sub*/) const
// {
//     //m_serviceSubscriptions.erase(sub.get());
//     //unsubscribeFromService(sub->getSubscriptionId());
// }

void Client2::sendAction(const Action& action, std::function<void(int32_t, std::string)> cb) const
{
#ifdef DEBUG_UPNP_CLIENT
    log::debug("Execute action: {}", action.getActionDocument().toString());
#endif

    http::Client client(m_loop);
    client.soapAction(action.getUrl(), action.getName(), action.getServiceTypeUrn(), action.toString(), std::move(cb));

#ifdef DEBUG_UPNP_CLIENT
    log::debug(result.toString());
#endif
}

// int Client2::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
// {
//     auto pClient = reinterpret_cast<Client*>(pCookie);

//     switch (eventType)
//     {
//     case UPNP_EVENT_RECEIVED:
//         pClient->UPnPEventOccurredEvent(reinterpret_cast<Upnp_Event*>(pEvent));
//         break;
//     default:
//         break;
//     }

//     return 0;
// }

}
