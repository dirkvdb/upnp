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

#include "upnp/upnprenderingcontrol.h"

#include "upnp/upnputils.h"
#include "upnp/upnpclient.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

static const int32_t defaultTimeout = 1801;
static const char* RenderingControlServiceType = "urn:schemas-upnp-org:service:RenderingControl:1";

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

RenderingControl::RenderingControl(Client& client)
: m_Client(client)
, m_currentVolume(0)
, m_maxVolume(100)
{
}

void RenderingControl::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
    
    if (m_Device->implementsService(ServiceType::RenderingControl))
    {
        parseServiceDescription(m_Device->m_Services[ServiceType::RenderingControl].m_SCPDUrl);
    }
}

void RenderingControl::subscribe()
{
    unsubscribe();
    
    m_Client.UPnPEventOccurredEvent.connect(std::bind(&RenderingControl::eventOccurred, this, _1), this);
    
    int ret = UpnpSubscribeAsync(m_Client, m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionURL.c_str(), defaultTimeout, &RenderingControl::eventCb, this);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device:" + numericops::toString(ret));
    }
}

void RenderingControl::unsubscribe()
{
    if (!m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID.empty())
    {
        m_Client.UPnPEventOccurredEvent.disconnect(this);
        
        int ret = UpnpUnSubscribe(m_Client, &(m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID[0]));
        if (ret != UPNP_E_SUCCESS)
        {
            log::warn("Failed to unsubscribe from device:", m_Device->m_FriendlyName);
        }
    }
}

IXmlDocument RenderingControl::executeAction(Action actionType, const std::string& connectionId)
{
    upnp::Action action(actionToString(actionType), RenderingControlServiceType);
    action.addArgument("InstanceID", connectionId);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::RenderingControl].m_ControlURL.c_str(), RenderingControlServiceType, nullptr, action.getActionDocument(), &result));
    
    return result;
}

IXmlDocument RenderingControl::executeAction(Action actionType, const std::string& connectionId, const std::map<std::string, std::string>& args)
{
    upnp::Action action(actionToString(actionType), RenderingControlServiceType);
    action.addArgument("InstanceID", connectionId);
    
    for (auto& arg : args)
    {
        action.addArgument(arg.first, arg.second);
    }
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[ServiceType::RenderingControl].m_ControlURL.c_str(), RenderingControlServiceType, nullptr, action.getActionDocument(), &result));
    
    return result;
}


void RenderingControl::increaseVolume(const std::string& connectionId, uint32_t percentage)
{
    int32_t vol = m_currentVolume + ((m_maxVolume * percentage) / 100);
    setVolume(connectionId, vol);
}

void RenderingControl::decreaseVolume(const std::string& connectionId, uint32_t percentage)
{
    int32_t vol = m_currentVolume - ((m_maxVolume * percentage) / 100);
    setVolume(connectionId, vol);
}

void RenderingControl::setVolume(const std::string& connectionId, int32_t value)
{
    numericops::clip(value, m_minVolume, m_maxVolume);
    executeAction(Action::SetVolume, connectionId, { {"Channel", "Master"}, { "DesiredVolume", numericops::toString(value)} });
}

void RenderingControl::parseServiceDescription(const std::string& descriptionUrl)
{
    IXmlDocument doc;
    
    int ret = UpnpDownloadXmlDoc(descriptionUrl.c_str(), &doc);
    if (ret != UPNP_E_SUCCESS)
    {
        log::error("Error obtaining device description from", descriptionUrl, " error =", ret);
        return;
    }
    
    for (auto& action : getActionsFromDescription(doc))
    {
        try
        {
            m_SupportedActions.insert(actionFromString(action));
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    }
    
    for (auto& variable : getStateVariablesFromDescription(doc))
    {
        if (variable.name == variableToString(Variable::Volume))
        {
            if (variable.valueRange)
            {
                m_minVolume = variable.valueRange->minimumValue;
                m_maxVolume = variable.valueRange->maximumValue;
            }
        }
    }
}

void RenderingControl::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 702: throw std::logic_error("Invalid instance id");
        default: upnp::handleUPnPResult(errorCode);
    }
}

void RenderingControl::eventOccurred(Upnp_Event* pEvent)
{
    if (pEvent->Sid == m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID)
    {
        try
        {
            IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(pEvent->ChangedVariables, "LastChange");
            if (!nodeList)
            {
                log::error("Failed to find LastChange element in RenderingControl event");
                return;
            }
            
            IXmlDocument doc = ixmlParseBuffer(getFirstElementValue(nodeList, "LastChange").c_str());
            if (!doc)
            {
                // empty lastchange
                return;
            }
            
            std::map<Variable, std::string> vars;
            auto values = getEventValues(doc);
            for (auto& i : values)
            {
                try
                {
                    vars[variableFromString(i.first)] = i.second;
                    log::debug(i.first, i.second);
                }
                catch (std::exception& e)
                {
                    log::warn("Unknown RenderingControl event variable ignored:", i.first);
                }
                
                auto iter = vars.find(Variable::Volume);
                if (iter != vars.end())
                {
                    m_currentVolume = stringops::toNumeric<uint32_t>(iter->second);
                }
            }
            
            LastChangedEvent(vars);
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse Rendering control event:", e.what());
        }
    }
}

int RenderingControl::eventCb(Upnp_EventType eventType, void* pEvent, void* pInstance)
{
    RenderingControl* rc = reinterpret_cast<RenderingControl*>(pInstance);
    
    switch (eventType)
    {
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        {
            Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
            if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
            {
                log::error("Error in Event Subscribe Callback:", pSubEvent->ErrCode);
            }
            else
            {
                if (pSubEvent->Sid)
                {
                    log::info(pSubEvent->Sid);
                    rc->m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID = pSubEvent->Sid;
                    log::info("Successfully subscribed to", rc->m_Device->m_FriendlyName, "id =", pSubEvent->Sid);
                }
                else
                {
                    rc->m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID.clear();
                    log::error("Subscription id for device is empty");
                }
            }
            break;
        }
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        {
            Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
            
            Upnp_SID newSID;
            int32_t timeout = defaultTimeout;
            int ret = UpnpSubscribe(rc->m_Client, pSubEvent->PublisherUrl, &timeout, newSID);
            if (ret == UPNP_E_SUCCESS)
            {
                rc->m_Device->m_Services[ServiceType::RenderingControl].m_EventSubscriptionID = newSID;
                log::info("RenderingControl subscription renewed: \n", newSID);
            }
            else
            {
                log::error("Failed to renew event subscription: ", ret);
            }
            break;
        }
        case UPNP_EVENT_RENEWAL_COMPLETE:
            log::info("RenderingControl subscription renewal complete");
            break;
        default:
            log::info("Unhandled action:", eventType);
            break;
    }
    
    return 0;
}

RenderingControl::Action RenderingControl::actionFromString(const std::string& action)
{
    if (action == "ListPresets")                return Action::ListPresets;
    if (action == "SelectPreset")               return Action::SelectPreset;
    if (action == "GetVolume")                  return Action::GetVolume;
    if (action == "SetVolume")                  return Action::SetVolume;
    if (action == "GetVolumeDB")                return Action::GetVolumeDB;
    if (action == "SetVolumeDB")                return Action::SetVolumeDB;
    if (action == "GetMute")                    return Action::GetMute;
    if (action == "SetMute")                    return Action::SetMute;
    
    throw std::logic_error("Unknown RendereringControl action:" + action);
}

std::string RenderingControl::actionToString(Action action)
{
    switch (action)
    {
    case Action::ListPresets:           return "ListPresets";
    case Action::SelectPreset:          return "SelectPreset";
    case Action::GetVolume:             return "GetVolume";
    case Action::SetVolume:             return "SetVolume";
    case Action::GetVolumeDB:           return "GetVolumeDB";
    case Action::SetVolumeDB:           return "SetVolumeDB";
    case Action::GetMute:               return "GetMute";
    case Action::SetMute:               return "SetMute";
    default:
        throw std::logic_error("Invalid RenderingControl action");
    }
}

RenderingControl::Variable RenderingControl::variableFromString(const std::string& var)
{
    if (var == "PresetNameList")                    return Variable::PresetNameList;
    if (var == "Mute")                              return Variable::Mute;
    if (var == "Volume")                            return Variable::Volume;
    if (var == "VolumeDB")                          return Variable::VolumeDB;
    if (var == "LastChange")                        return Variable::LastChange;
    
    throw std::logic_error("Unknown RenderingControl variable:" + var);
}

std::string RenderingControl::variableToString(Variable var)
{
    switch (var)
    {
        case Variable::PresetNameList:              return "PresetNameList";
        case Variable::Mute:                        return "Mute";
        case Variable::Volume:                      return "Volume";
        case Variable::VolumeDB:                    return "VolumeDB";
        case Variable::LastChange:                  return "LastChange";
        default:
            throw std::logic_error("Invalid RenderingControl variable");
    }
}

}
