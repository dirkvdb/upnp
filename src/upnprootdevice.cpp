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

#include "utils/log.h"

using namespace utils;

namespace upnp
{

RootDevice::RootDevice(const std::string& udn, const std::string& descriptionXml, int32_t advertiseIntervalInSeconds)
: m_device(0)
, m_udn(udn)
, m_descriptionXml(descriptionXml)
, m_advertiseInterval(advertiseIntervalInSeconds)
{
}

RootDevice::~RootDevice()
{
    try
    {
        destroy();
    }
    catch (std::exception&) {}
}

void RootDevice::initialize()
{
    handleUPnPResult(UpnpRegisterRootDevice2(UPNPREG_BUF_DESC, m_descriptionXml.c_str(), m_descriptionXml.size() + 1, 1, RootDevice::upnpCallback, this, &m_device));
    handleUPnPResult(UpnpSendAdvertisement(m_device, m_advertiseInterval));
}

void RootDevice::destroy()
{
    if (m_device != 0)
    {
        log::debug("Unregister root device");
        handleUPnPResult(UpnpUnRegisterRootDevice(m_device));
        m_device = 0;
    }
}

std::string RootDevice::getUniqueDeviceName()
{
    return m_udn;
}

void RootDevice::acceptSubscription(const std::string& serviceId, const std::string& subscriptionId, const xml::Document& response)
{
    try
    {
                                                                                                         // UPnP API bug, should be const ([in] argument)
        handleUPnPResult(UpnpAcceptSubscriptionExt(m_device, m_udn.c_str(), serviceId.c_str(), response, const_cast<char*>(subscriptionId.c_str())));
    }
    catch (std::exception& e)
    {
        log::warn("Failed to accept subscription: {}", e.what());
    }
}

void RootDevice::notifyEvent(const std::string& serviceId, const xml::Document& event)
{
    handleUPnPResult(UpnpNotifyExt(m_device, m_udn.c_str(), serviceId.c_str(), event));
}

int RootDevice::upnpCallback(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    auto dev = reinterpret_cast<RootDevice*>(pCookie);

    switch (eventType)
	{
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        {
            auto request = reinterpret_cast<Upnp_Subscription_Request*>(pEvent);
            if (request->UDN == nullptr || request->ServiceId == nullptr)
            {
                log::warn("Invalid Event subscription request: NULL value provided");
                return -1;
            }

            dev->EventSubscriptionRequested(request);
			break;
        }
		case UPNP_CONTROL_GET_VAR_REQUEST:
			log::warn("Deprecated: UPNP_CONTROL_GET_VAR_REQUEST");
			break;
		case UPNP_CONTROL_ACTION_REQUEST:
        {
            auto request = reinterpret_cast<Upnp_Action_Request*>(pEvent);

            try
            {
                // The appropriate service should fill in the result
                dev->ControlActionRequested(request);
            }
            catch (Exception& e)
            {
                assert(strlen(e.what()) < 180);

                log::warn("Error processing request: {}", e.what());
                request->ErrCode = e.getErrorCode();
                strcpy(request->ErrStr, e.what());
            }

			break;
        }
		default:
			log::error("RootDevice: Unknown eventType {}", eventType);
	}

    return 0;
}

}
