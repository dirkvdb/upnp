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

#include "upnp/upnpmediarendererdevice.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

#include <sstream>

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

MediaRendererDevice::MediaRendererDevice(const std::string& descriptionXml, int32_t advertiseIntervalInSeconds, IConnectionManager& cm, IRenderingControl& rc)
: RootDevice(descriptionXml, advertiseIntervalInSeconds)
{
    m_Services.insert(std::make_pair(ServiceType::RenderingControl, std::unique_ptr<RenderingControlService>(new RenderingControlService(rc))));
    m_Services.insert(std::make_pair(ServiceType::ConnectionManager, std::unique_ptr<ConnectionManagerService>(new ConnectionManagerService(cm))));
}
    
void MediaRendererDevice::onEventSubscriptionRequest(const std::string& udn, const std::string& serviceId, const std::string& subscriptionId)
{
    log::debug("Renderer: event subscription request %s", serviceId);
    
    try
    {
        auto& service = m_Services.at(serviceIdUrnStringToService(serviceId));
        AcceptSubscription(udn, serviceId, subscriptionId, service->getVariables());
    }
    catch (std::out_of_range&)
    {
        log::warn("Invalid event subscription request: %s", serviceId);
    }
}

ActionResponse MediaRendererDevice::onControlActionRequest(const std::string& udn, const std::string& serviceId, ActionRequest& request)
{
    log::debug("Renderer: action request: %s", request.actionName);
    log::debug(request.request.toString());
    
    try
    {
        auto& service = m_Services.at(serviceIdUrnStringToService(serviceId));
        return service->onAction(request.actionName, request.request);
    }
    catch (std::out_of_range&)
    {
        throw ServiceException("Invalid subscribtionId", 401);
    }
}
    
}
