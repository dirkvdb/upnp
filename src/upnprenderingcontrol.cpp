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
#include "upnp/upnpclientinterface.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

static const int32_t defaultTimeout = 1801;

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

RenderingControl::RenderingControl(IClient& client)
: m_Client(client)
, m_CurrentVolume(0)
, m_MinVolume(0)
, m_MaxVolume(100)
{
}

void RenderingControl::setDevice(const std::shared_ptr<Device>& device)
{
    if (device->implementsService(ServiceType::RenderingControl))
    {
        m_Service = device->m_Services[ServiceType::RenderingControl];
        parseServiceDescription(m_Service.m_SCPDUrl);
    }
}

void RenderingControl::subscribe()
{
    unsubscribe();
    
    m_Client.UPnPEventOccurredEvent.connect(std::bind(&RenderingControl::eventOccurred, this, _1), this);
    m_Client.subscribeToService(m_Service.m_EventSubscriptionURL, defaultTimeout, &RenderingControl::eventCb, this);
}

void RenderingControl::unsubscribe()
{
    if (!m_Service.m_EventSubscriptionID.empty())
    {
        m_Client.UPnPEventOccurredEvent.disconnect(this);
        m_Client.unsubscribeFromService(&(m_Service.m_EventSubscriptionID[0]));
        m_Service.m_EventSubscriptionID.clear();
    }
}

bool RenderingControl::supportsAction(Action action)
{
    return m_SupportedActions.find(action) != m_SupportedActions.end();
}

xml::Document RenderingControl::executeAction(Action actionType, const std::string& connectionId, const std::map<std::string, std::string>& args)
{
    upnp::Action action(actionToString(actionType), m_Service.m_ControlURL, ServiceType::RenderingControl);
    action.addArgument("InstanceID", connectionId);
    
    for (auto& arg : args)
    {
        action.addArgument(arg.first, arg.second);
    }
    
    try
    {
        return m_Client.sendAction(action);
    }
    catch (int32_t errorCode)
    {
        handleUPnPResult(errorCode);
    }
    
    assert(false);
    return xml::Document();
}


void RenderingControl::increaseVolume(const std::string& connectionId, uint32_t percentage)
{
    int32_t vol = m_CurrentVolume + ((m_MaxVolume * percentage) / 100);
    setVolume(connectionId, vol);
}

void RenderingControl::decreaseVolume(const std::string& connectionId, uint32_t percentage)
{
    int32_t vol = m_CurrentVolume - ((m_MaxVolume * percentage) / 100);
    setVolume(connectionId, vol);
}

void RenderingControl::setVolume(const std::string& connectionId, int32_t value)
{
    numericops::clip(value, m_MinVolume, m_MaxVolume);
    executeAction(Action::SetVolume, connectionId, { {"Channel", "Master"}, { "DesiredVolume", numericops::toString(value)} });
}

void RenderingControl::parseServiceDescription(const std::string& descriptionUrl)
{
    try
    {
        xml::Document doc = m_Client.downloadXmlDocument(descriptionUrl);
        
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
                    m_MinVolume = variable.valueRange->minimumValue;
                    m_MaxVolume = variable.valueRange->maximumValue;
                }
            }
        }
    }
    catch (std::exception& e)
    {
        log::error(e.what());
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
    if (pEvent->Sid == m_Service.m_EventSubscriptionID)
    {
        try
        {
            xml::Document doc(pEvent->ChangedVariables, xml::Document::NoOwnership);
            xml::Document changeDoc(doc.getChildElementValueRecursive("LastChange"));
            
            //log::debug(doc.toString());
            
            xml::Node instanceNode = changeDoc.getElementsByTagName("InstanceID").getNode(0);
            xml::NodeList children = instanceNode.getChildNodes();
            
            std::map<Variable, std::string> vars;
            unsigned long numVars = children.size();
            for (unsigned long i = 0; i < numVars; ++i)
            {
                try
                {
                    xml::Element elem = children.getNode(i);
                    vars.insert(std::make_pair(variableFromString(elem.getName()), elem.getAttribute("val")));
                }
                catch (std::exception& e)
                {
                    log::warn("Unknown RenderingControl event variable ignored:", e.what());
                }
            }
            
            auto iter = vars.find(Variable::Volume);
            if (iter != vars.end())
            {
                m_CurrentVolume = stringops::toNumeric<uint32_t>(iter->second);
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
                    rc->m_Service.m_EventSubscriptionID = pSubEvent->Sid;
                }
                else
                {
                    rc->m_Service.m_EventSubscriptionID.clear();
                    log::error("Subscription id for device is empty");
                }
            }
            break;
        }
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        {
            Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
            
            try
            {
                Upnp_SID newSID;
                int32_t timeout = defaultTimeout;
                
                rc->m_Client.subscribeToService(pSubEvent->PublisherUrl, timeout, newSID);
                rc->m_Service.m_EventSubscriptionID = newSID;
                
                log::debug("RenderingControl subscription renewed: \n", newSID);
            }
            catch (std::exception& e)
            {
                log::error(std::string("Failed to renew RenderingControl event subscription: ") + e.what());
            }
            break;
        }
        case UPNP_EVENT_RENEWAL_COMPLETE:
            log::debug("RenderingControl subscription renewal complete");
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
