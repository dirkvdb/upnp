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

void MediaRendererDevice::onControlActionRequest(const std::string& udn, const std::string& serviceId, ActionRequest& request)
{
    log::debug("Renderer: action request: %s", request.actionName);
    log::debug(request.request.toString());
    
    try
    {
        auto& service = m_Services.at(serviceIdUrnStringToService(serviceId));
        //request.result = service->onAction(request.actionName).getActionDocument();
    }
    catch (std::out_of_range&)
    {
        request.errorCode           = 401;
        request.errorString         = "Invalid subscribtionId";
    }
    catch (ServiceException& e)
    {
        log::warn("Error processing request: %s", e.what());
        request.errorCode           = e.errorCode();
        request.errorString         = e.what();
    }
}
    
//    if (!action_found) {
//		ca_event->ActionResult = NULL;
//		strcpy(ca_event->ErrStr, "Invalid Action");
//		ca_event->ErrCode = 401;
//	} else {
//		if (retCode == UPNP_E_SUCCESS) {
//			ca_event->ErrCode = UPNP_E_SUCCESS;
//		} else {
//			/* copy the error string */
//			strcpy(ca_event->ErrStr, errorString);
//			switch (retCode) {
//			case UPNP_E_INVALID_PARAM:
//				ca_event->ErrCode = 402;
//				break;
//			case UPNP_E_INTERNAL_ERROR:
//			default:
//				ca_event->ErrCode = 501;
//				break;
//			}
//		}
//	}
//}


}
