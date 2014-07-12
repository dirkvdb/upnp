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

#include "upnp/upnprenderingcontrolservice.h"

#include "utils/log.h"

using namespace utils;

namespace upnp
{
namespace RenderingControl
{

Service::Service(IRootDevice& dev, IRenderingControl& rc)
: DeviceService(dev, ServiceType::RenderingControl)
, m_RenderingControl(rc)
, m_LastChange(m_Type, std::chrono::milliseconds(200))
{
    m_LastChange.LastChangeEvent = [this] (const xml::Document& doc) {
        m_RootDevice.notifyEvent(serviceTypeToUrnIdString(m_Type), doc);
    };
}

Service::~Service()
{
    m_LastChange.LastChangeEvent = nullptr;
}

void Service::updateAudioVariable(std::map<uint32_t, std::map<Channel, ServiceVariable>>& vars, uint32_t instanceId, Channel channel, Variable var, const std::string& value)
{
    try
    {
        if (vars.at(instanceId).at(channel).getValue() == value)
        {
            // no value change
            return;
        }
    }
    catch (std::out_of_range&) { /* variable did not exist yet, add it */ }
    
    ServiceVariable serviceVar(toString(var), value);
    serviceVar.addAttribute("channel", toString(channel));

    vars[instanceId][channel] = serviceVar;
    m_LastChange.addChangedVariable(instanceId, serviceVar);
}

void Service::setMute(uint32_t instanceId, Channel channel, bool enabled)
{
    updateAudioVariable(m_Mute, instanceId, channel, Variable::Mute, enabled ? "1" : "0");
}

void Service::setLoudness(uint32_t instanceId, Channel channel, bool enabled)
{
    updateAudioVariable(m_Loudness, instanceId, channel, Variable::Loudness, enabled ? "1" : "0");
}

void Service::setVolume(uint32_t instanceId, Channel channel, uint16_t volume)
{
    updateAudioVariable(m_Volumes, instanceId, channel, Variable::Volume, std::to_string(volume));
    // TODO update corresponding volumedb value
}

void Service::setVolumeDB(uint32_t instanceId, Channel channel, int16_t volume)
{
    updateAudioVariable(m_DBVolumes, instanceId, channel, Variable::VolumeDB, std::to_string(volume));
    // TODO update corresponding volume value
}

static void addChannelVariables(uint32_t instanceId, std::stringstream& ss, const std::map<uint32_t, std::map<Channel, ServiceVariable>>& vars)
{
    try
    {
        for (auto& var : vars.at(instanceId))
        {
            ss << var.second.toString();
        }
    }
    catch (std::out_of_range&) {}
}

xml::Document Service::getSubscriptionResponse()
{
    const std::string ns = "urn:schemas-upnp-org:event-1-0";

    xml::Document doc;
    auto propertySet    = doc.createElement("e:propertyset");
    auto property       = doc.createElement("e:property");
    auto lastChange     = doc.createElement("LastChange");
    
    propertySet.addAttribute("xmlns:e", ns);
    
    std::stringstream ss;
    ss << "<Event xmlns=\"" << serviceTypeToUrnMetadataString(m_Type) << "\">" << std::endl;
    
    for (auto& vars : m_Variables)
    {
        ss << "<InstanceID val=\"" << vars.first << "\">";
    
        for (auto& var : vars.second)
        {
            ss << var.second.toString();
        }
        
        // also add the audio values which have a value per channel
        addChannelVariables(vars.first, ss, m_Volumes);
        addChannelVariables(vars.first, ss, m_DBVolumes);
        addChannelVariables(vars.first, ss, m_Mute);
        addChannelVariables(vars.first, ss, m_Loudness);
        
        ss << "</InstanceID>";
    }
    
    ss << "</Event>";


    auto lastChangeValue = doc.createNode(ss.str());

    lastChange.appendChild(lastChangeValue);
    property.appendChild(lastChange);
    propertySet.appendChild(property);
    doc.appendChild(propertySet);

#ifdef DEBUG_RENDERING_CONTROL
    log::debug(doc.toString());
#endif
    
    return doc;
}

ActionResponse Service::onAction(const std::string& action, const xml::Document& doc)
{
    try
    {
        ActionResponse response(action, ServiceType::RenderingControl);
        auto req = doc.getFirstChild();
        uint32_t id = static_cast<uint32_t>(std::stoul(req.getChildNodeValue("InstanceID")));
        
        switch (actionFromString(action))
        {
        case Action::ListPresets:
            response.addArgument("CurrentPresetNameList", getInstanceVariable(id, Variable::PresetNameList).getValue());
            break;
        case Action::SelectPreset:
            m_RenderingControl.selectPreset(id, req.getChildNodeValue("PresetName"));
            break;
        case Action::GetBrightness:
            response.addArgument("CurrentBrightness", getInstanceVariable(id, Variable::Brightness).getValue());
            break;
        case Action::SetBrightness:
            m_RenderingControl.setBrightness(id, std::stoi(req.getChildNodeValue("DesiredBrightness")));
            break;
        case Action::GetContrast:
            response.addArgument("CurrentContrast", getInstanceVariable(id, Variable::Contrast).getValue());
            break;
        case Action::SetContrast:
            m_RenderingControl.setContrast(id, std::stoi(req.getChildNodeValue("DesiredContrast")));
            break;
        case Action::GetSharpness:
            response.addArgument("CurrentSharpness", getInstanceVariable(id, Variable::Sharpness).getValue());
            break;
        case Action::SetSharpness:
            m_RenderingControl.setSharpness(id, std::stoi(req.getChildNodeValue("DesiredSharpness")));
            break;
        case Action::GetRedVideoGain:
            response.addArgument("CurrentContrast", getInstanceVariable(id, Variable::RedVideoGain).getValue());
            break;
        case Action::SetRedVideoGain:
            m_RenderingControl.setRedVideoGain(id, std::stoi(req.getChildNodeValue("DesiredRedVideoGain")));
            break;
        case Action::GetGreenVideoGain:
            response.addArgument("CurrentGreenVideoGain", getInstanceVariable(id, Variable::GreenVideoGain).getValue());
            break;
        case Action::SetGreenVideoGain:
            m_RenderingControl.setGreenVideoGain(id, std::stoi(req.getChildNodeValue("DesiredGreenVideoGain")));
            break;
        case Action::GetBlueVideoGain:
            response.addArgument("CurrentBlueVideoGain", getInstanceVariable(id, Variable::BlueVideoGain).getValue());
            break;
        case Action::SetBlueVideoGain:
            m_RenderingControl.setBlueVideoGain(id, std::stoi(req.getChildNodeValue("DesiredBlueVideoGain")));
            break;
        case Action::GetRedVideoBlackLevel:
            response.addArgument("CurrentRedVideoBlackLevel", getInstanceVariable(id, Variable::RedVideoBlackLevel).getValue());
            break;
        case Action::SetRedVideoBlackLevel:
            m_RenderingControl.setRedVideoBlackLevel(id, std::stoi(req.getChildNodeValue("DesiredRedVideoBlackLevel")));
            break;
        case Action::GetGreenVideoBlackLevel:
            response.addArgument("CurrentGreenVideoBlackLevel", getInstanceVariable(id, Variable::GreenVideoBlackLevel).getValue());
            break;
        case Action::SetGreenVideoBlackLevel:
            m_RenderingControl.setGreenVideoBlackLevel(id, std::stoi(req.getChildNodeValue("DesiredGreenVideoBlackLevel")));
            break;
        case Action::GetBlueVideoBlackLevel:
            response.addArgument("CurrentBlueVideoBlackLevel", getInstanceVariable(id, Variable::BlueVideoBlackLevel).getValue());
            break;
        case Action::SetBlueVideoBlackLevel:
            m_RenderingControl.setBlueVideoBlackLevel(id, std::stoi(req.getChildNodeValue("DesiredBlueVideoBlackLevel")));
            break;
        case Action::GetColorTemperature:
            response.addArgument("CurrentColorTemperature", getInstanceVariable(id, Variable::ColorTemperature).getValue());
            break;
        case Action::SetColorTemperature:
            m_RenderingControl.setColorTemperature(id, std::stoi(req.getChildNodeValue("DesiredColorTemperature")));
            break;
        case Action::GetHorizontalKeystone:
            response.addArgument("CurrentHorizontalKeystone", getInstanceVariable(id, Variable::HorizontalKeystone).getValue());
            break;
        case Action::SetHorizontalKeystone:
            m_RenderingControl.setHorizontalKeystone(id, std::stoi(req.getChildNodeValue("DesiredHorizontalKeystone")));
            break;
        case Action::GetVerticalKeystone:
            response.addArgument("CurrentVerticalKeystone", getInstanceVariable(id, Variable::VerticalKeystone).getValue());
            break;
        case Action::SetVerticalKeystone:
            m_RenderingControl.setVerticalKeystone(id, std::stoi(req.getChildNodeValue("DesiredVerticalKeystone")));
            break;
        case Action::GetMute:
            response.addArgument("CurrentMute", m_Mute.at(id)[channelFromString(req.getChildNodeValue("Channel"))].getValue());
            break;
        case Action::SetMute:
            m_RenderingControl.setMute(id, channelFromString(req.getChildNodeValue("Channel")), req.getChildNodeValue("DesiredMute") == "1");
            break;
        case Action::GetVolume:
            response.addArgument("CurrentVolume", m_Volumes.at(id)[channelFromString(req.getChildNodeValue("Channel"))].getValue());
            break;
        case Action::SetVolume:
            m_RenderingControl.setVolume(id, channelFromString(req.getChildNodeValue("Channel")), std::stoi(req.getChildNodeValue("DesiredVolume")));
            break;
        case Action::GetVolumeDB:
            response.addArgument("CurrentVolumeDB", m_DBVolumes.at(id)[channelFromString(req.getChildNodeValue("Channel"))].getValue());
            break;
        case Action::SetVolumeDB:
            m_RenderingControl.setVolumeDB(id, channelFromString(req.getChildNodeValue("Channel")), std::stoi(req.getChildNodeValue("DesiredVolumeDB")));
            break;
        case Action::GetVolumeDBRange:
        {
            auto range = m_RenderingControl.getVolumeDBRange(id, channelFromString(req.getChildNodeValue("Channel")));
            response.addArgument("MinValue", std::to_string(range.first));
            response.addArgument("MaxValue", std::to_string(range.second));
            break;
        }
        case Action::GetLoudness:
            response.addArgument("CurrentLoudness", m_Loudness.at(id)[channelFromString(req.getChildNodeValue("Channel"))].getValue());
            break;
        case Action::SetLoudness:
            m_RenderingControl.setLoudness(id, channelFromString(req.getChildNodeValue("Channel")), req.getChildNodeValue("DesiredLoudness") == "1");
            break;
        default:
            log::warn("No handler for RenderingControl action: %s", action);
            throw InvalidActionException();
        }
    
        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing RenderingControl request: %s", e.what());
        throw InvalidActionException();
    }
}

void Service::setInstanceVariable(uint32_t id, Variable var, const std::string& value)
{
    DeviceService::setInstanceVariable(id, var, value);
    m_LastChange.addChangedVariable(id, ServiceVariable(toString(var), value));
}

std::string Service::variableToString(Variable type) const
{
    return RenderingControl::toString(type);
}

}
}
