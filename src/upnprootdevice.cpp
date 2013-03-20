//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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


#include "upnp/upnprootdevice.h"
#include "upnp/upnputils.h"
#include <upnp/upnp.h>

#include "utils/log.h"

using namespace utils;

namespace upnp
{

RootDevice::RootDevice(const std::string& descriptionXml, int32_t advertiseIntervalInSeconds)
: m_device(0)
{
    handleUPnPResult(UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, descriptionXml.c_str(), descriptionXml.size() + 1, 1, RootDevice::upnpCallback, this, &m_device));
    handleUPnPResult(UpnpSendAdvertisement(m_device, advertiseIntervalInSeconds));
}
    
RootDevice::~RootDevice()
{
    if (m_device != 0)
    {
        try
        {
            log::debug("Unregister root device");
            handleUPnPResult(UpnpUnRegisterRootDevice(m_device));
        }
        catch (std::exception& e)
        {
            log::error("Failed to unregister root device: %s", e.what());
        }
    }
}

void RootDevice::AcceptSubscription(const std::string& udn, const std::string& serviceId, const std::string& subscriptionId, const std::map<std::string, std::string>& vars)
{
    const char** names = new const char*[vars.size()];
    const char** values = new const char*[vars.size()];
    
    int32_t i = 0;
    for (auto& iter : vars)
    {
        names[i]    = iter.first.c_str();
        names[i++]  = iter.second.c_str();
    }
    
    try
    {
        handleUPnPResult(UpnpAcceptSubscription(m_device, udn.c_str(), serviceId.c_str(), names, values, vars.size(), subscriptionId.c_str()));
    }
    catch (std::exception& e)
    {
        log::warn("Failed to accept subscription: %s", e.what());
    }
    
    delete [] names;
    delete [] values;
}
                     
int RootDevice::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    auto dev = reinterpret_cast<RootDevice*>(pCookie);

    switch (eventType)
	{
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        {
            auto request = reinterpret_cast<Upnp_Subscription_Request*>(pEvent);
            dev->onEventSubscriptionRequest(request->UDN, request->ServiceId, request->Sid);
			break;
        }
		case UPNP_CONTROL_GET_VAR_REQUEST:
			log::warn("Deprecated: UPNP_CONTROL_GET_VAR_REQUEST");
			break;
		case UPNP_CONTROL_ACTION_REQUEST:
        {
            auto request = reinterpret_cast<Upnp_Action_Request*>(pEvent);
            
            ActionRequest req;
            req.actionName  = request->ActionName;
            req.request     = xml::Document(request->ActionRequest, xml::Document::NoOwnership);
            req.result      = xml::Document(request->ActionResult, xml::Document::NoOwnership);
            
            dev->onControlActionRequest(request->DevUDN, request->ServiceID, req);
			break;
        }
		default:
			log::error("RootDevice: Unknown eventType %d", eventType);
	}
    
    return 0;
}
    
}
