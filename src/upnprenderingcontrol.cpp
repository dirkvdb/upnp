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

using namespace utils;
using namespace std::placeholders;

namespace upnp
{

static const int32_t g_subscriptionTimeout = 1801;

// TODO: assign min and max volume

RenderingControl::RenderingControl(IClient& client)
: ServiceBase(client)
, m_CurrentVolume(0)
, m_MinVolume(0)
, m_MaxVolume(100)
{
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
    executeAction(RenderingControlAction::SetVolume, connectionId, { {"Channel", "Master"}, { "DesiredVolume", numericops::toString(value)} });
}

ServiceType RenderingControl::getType()
{
    return ServiceType::RenderingControl;
}

int32_t RenderingControl::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

void RenderingControl::parseServiceDescription(const std::string& descriptionUrl)
{
    ServiceBase::parseServiceDescription(descriptionUrl);
    
    for (auto& variable : m_StateVariables)
    {
        if (variable.name == variableToString(RenderingControlVariable::Volume))
        {
            if (variable.valueRange)
            {
                m_MinVolume = variable.valueRange->minimumValue;
                m_MaxVolume = variable.valueRange->maximumValue;
            }
        }
    }
}

void RenderingControl::handleLastChangeEvent(const std::map<RenderingControlVariable, std::string>& variables)
{
    auto iter = variables.find(RenderingControlVariable::Volume);
    if (iter != variables.end())
    {
        m_CurrentVolume = stringops::toNumeric<uint32_t>(iter->second);
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

RenderingControlAction RenderingControl::actionFromString(const std::string& action)
{
    if (action == "ListPresets")                return RenderingControlAction::ListPresets;
    if (action == "SelectPreset")               return RenderingControlAction::SelectPreset;
    if (action == "GetVolume")                  return RenderingControlAction::GetVolume;
    if (action == "SetVolume")                  return RenderingControlAction::SetVolume;
    if (action == "GetVolumeDB")                return RenderingControlAction::GetVolumeDB;
    if (action == "SetVolumeDB")                return RenderingControlAction::SetVolumeDB;
    if (action == "GetMute")                    return RenderingControlAction::GetMute;
    if (action == "SetMute")                    return RenderingControlAction::SetMute;
    
    throw std::logic_error("Unknown RendereringControl action:" + action);
}

std::string RenderingControl::actionToString(RenderingControlAction action)
{
    switch (action)
    {
    case RenderingControlAction::ListPresets:           return "ListPresets";
    case RenderingControlAction::SelectPreset:          return "SelectPreset";
    case RenderingControlAction::GetVolume:             return "GetVolume";
    case RenderingControlAction::SetVolume:             return "SetVolume";
    case RenderingControlAction::GetVolumeDB:           return "GetVolumeDB";
    case RenderingControlAction::SetVolumeDB:           return "SetVolumeDB";
    case RenderingControlAction::GetMute:               return "GetMute";
    case RenderingControlAction::SetMute:               return "SetMute";
    default:
        throw std::logic_error("Invalid RenderingControl action");
    }
}

RenderingControlVariable RenderingControl::variableFromString(const std::string& var)
{
    if (var == "PresetNameList")                    return RenderingControlVariable::PresetNameList;
    if (var == "Mute")                              return RenderingControlVariable::Mute;
    if (var == "Volume")                            return RenderingControlVariable::Volume;
    if (var == "VolumeDB")                          return RenderingControlVariable::VolumeDB;
    if (var == "LastChange")                        return RenderingControlVariable::LastChange;
    
    throw std::logic_error("Unknown RenderingControl variable:" + var);
}

std::string RenderingControl::variableToString(RenderingControlVariable var)
{
    switch (var)
    {
        case RenderingControlVariable::PresetNameList:              return "PresetNameList";
        case RenderingControlVariable::Mute:                        return "Mute";
        case RenderingControlVariable::Volume:                      return "Volume";
        case RenderingControlVariable::VolumeDB:                    return "VolumeDB";
        case RenderingControlVariable::LastChange:                  return "LastChange";
        default:
            throw std::logic_error("Invalid RenderingControl variable");
    }
}

}
