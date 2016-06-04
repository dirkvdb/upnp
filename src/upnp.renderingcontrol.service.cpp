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

#include "upnp/upnp.renderingcontrol.service.h"
#include "upnp.renderingcontrol.typeconversions.h"
#include "utils/log.h"
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml.hpp"

namespace upnp
{
namespace RenderingControl
{

using namespace utils;
using namespace rapidxml_ns;

Service::Service(IRootDevice& dev, IRenderingControl& rc)
: DeviceService(dev, { ServiceType::RenderingControl, 1 })
, m_renderingControl(rc)
, m_lastChange(m_type, std::chrono::milliseconds(200))
{
    m_lastChange.LastChangeEvent = [this] (const std::string& notification) {
        m_rootDevice.notifyEvent(serviceTypeToUrnIdString(m_type), notification);
    };
}

Service::~Service()
{
    m_lastChange.LastChangeEvent = nullptr;
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
    m_lastChange.addChangedVariable(instanceId, serviceVar);
}

void Service::setMute(uint32_t instanceId, Channel channel, bool enabled)
{
    updateAudioVariable(m_mute, instanceId, channel, Variable::Mute, enabled ? "1" : "0");
}

void Service::setLoudness(uint32_t instanceId, Channel channel, bool enabled)
{
    updateAudioVariable(m_loudness, instanceId, channel, Variable::Loudness, enabled ? "1" : "0");
}

void Service::setVolume(uint32_t instanceId, Channel channel, uint16_t volume)
{
    updateAudioVariable(m_volumes, instanceId, channel, Variable::Volume, std::to_string(volume));
    // TODO update corresponding volumedb value
}

void Service::setVolumeDB(uint32_t instanceId, Channel channel, int16_t volume)
{
    updateAudioVariable(m_dbVolumes, instanceId, channel, Variable::VolumeDB, std::to_string(volume));
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

std::string Service::getSubscriptionResponse()
{
    // TODO: avoid the copies

    std::stringstream ss;
    ss << "<Event xmlns=\"" << serviceTypeToUrnMetadataString(m_type) << "\">" << std::endl;

    for (auto& vars : m_variables)
    {
        ss << "<InstanceID val=\"" << vars.first << "\">";

        for (auto& var : vars.second)
        {
            ss << var.second.toString();
        }

        // also add the audio values which have a value per channel
        addChannelVariables(vars.first, ss, m_volumes);
        addChannelVariables(vars.first, ss, m_dbVolumes);
        addChannelVariables(vars.first, ss, m_mute);
        addChannelVariables(vars.first, ss, m_loudness);

        ss << "</InstanceID>";
    }

    ss << "</Event>";

    std::vector<std::pair<std::string, std::string>> vars;
    vars.emplace_back("LastChange", ss.str());
    auto doc = xml::createNotificationXml(vars);

#ifdef DEBUG_RENDERING_CONTROL
    log::debug(doc);
#endif

    return doc;
}

ActionResponse Service::onAction(const std::string& action, const std::string& requestDoc)
{
    try
    {
        xml_document<> doc;
        doc.parse<parse_non_destructive | parse_trim_whitespace>(requestDoc.c_str());

        ActionResponse response(action, { ServiceType::RenderingControl, 1 });
        auto& req = doc.first_node_ref();
        uint32_t id = static_cast<uint32_t>(std::stoul(xml::requiredChildValue(req, "InstanceID")));

        switch (actionFromString(action))
        {
        case Action::ListPresets:
            response.addArgument("CurrentPresetNameList", getInstanceVariable(id, Variable::PresetNameList).getValue());
            break;
        case Action::SelectPreset:
            m_renderingControl.selectPreset(id, xml::requiredChildValue(req, "PresetName"));
            break;
        case Action::GetBrightness:
            response.addArgument("CurrentBrightness", getInstanceVariable(id, Variable::Brightness).getValue());
            break;
        case Action::SetBrightness:
            m_renderingControl.setBrightness(id, std::stoi(xml::requiredChildValue(req, "DesiredBrightness")));
            break;
        case Action::GetContrast:
            response.addArgument("CurrentContrast", getInstanceVariable(id, Variable::Contrast).getValue());
            break;
        case Action::SetContrast:
            m_renderingControl.setContrast(id, std::stoi(xml::requiredChildValue(req, "DesiredContrast")));
            break;
        case Action::GetSharpness:
            response.addArgument("CurrentSharpness", getInstanceVariable(id, Variable::Sharpness).getValue());
            break;
        case Action::SetSharpness:
            m_renderingControl.setSharpness(id, std::stoi(xml::requiredChildValue(req, "DesiredSharpness")));
            break;
        case Action::GetRedVideoGain:
            response.addArgument("CurrentContrast", getInstanceVariable(id, Variable::RedVideoGain).getValue());
            break;
        case Action::SetRedVideoGain:
            m_renderingControl.setRedVideoGain(id, std::stoi(xml::requiredChildValue(req, "DesiredRedVideoGain")));
            break;
        case Action::GetGreenVideoGain:
            response.addArgument("CurrentGreenVideoGain", getInstanceVariable(id, Variable::GreenVideoGain).getValue());
            break;
        case Action::SetGreenVideoGain:
            m_renderingControl.setGreenVideoGain(id, std::stoi(xml::requiredChildValue(req, "DesiredGreenVideoGain")));
            break;
        case Action::GetBlueVideoGain:
            response.addArgument("CurrentBlueVideoGain", getInstanceVariable(id, Variable::BlueVideoGain).getValue());
            break;
        case Action::SetBlueVideoGain:
            m_renderingControl.setBlueVideoGain(id, std::stoi(xml::requiredChildValue(req, "DesiredBlueVideoGain")));
            break;
        case Action::GetRedVideoBlackLevel:
            response.addArgument("CurrentRedVideoBlackLevel", getInstanceVariable(id, Variable::RedVideoBlackLevel).getValue());
            break;
        case Action::SetRedVideoBlackLevel:
            m_renderingControl.setRedVideoBlackLevel(id, std::stoi(xml::requiredChildValue(req, "DesiredRedVideoBlackLevel")));
            break;
        case Action::GetGreenVideoBlackLevel:
            response.addArgument("CurrentGreenVideoBlackLevel", getInstanceVariable(id, Variable::GreenVideoBlackLevel).getValue());
            break;
        case Action::SetGreenVideoBlackLevel:
            m_renderingControl.setGreenVideoBlackLevel(id, std::stoi(xml::requiredChildValue(req, "DesiredGreenVideoBlackLevel")));
            break;
        case Action::GetBlueVideoBlackLevel:
            response.addArgument("CurrentBlueVideoBlackLevel", getInstanceVariable(id, Variable::BlueVideoBlackLevel).getValue());
            break;
        case Action::SetBlueVideoBlackLevel:
            m_renderingControl.setBlueVideoBlackLevel(id, std::stoi(xml::requiredChildValue(req, "DesiredBlueVideoBlackLevel")));
            break;
        case Action::GetColorTemperature:
            response.addArgument("CurrentColorTemperature", getInstanceVariable(id, Variable::ColorTemperature).getValue());
            break;
        case Action::SetColorTemperature:
            m_renderingControl.setColorTemperature(id, std::stoi(xml::requiredChildValue(req, "DesiredColorTemperature")));
            break;
        case Action::GetHorizontalKeystone:
            response.addArgument("CurrentHorizontalKeystone", getInstanceVariable(id, Variable::HorizontalKeystone).getValue());
            break;
        case Action::SetHorizontalKeystone:
            m_renderingControl.setHorizontalKeystone(id, std::stoi(xml::requiredChildValue(req, "DesiredHorizontalKeystone")));
            break;
        case Action::GetVerticalKeystone:
            response.addArgument("CurrentVerticalKeystone", getInstanceVariable(id, Variable::VerticalKeystone).getValue());
            break;
        case Action::SetVerticalKeystone:
            m_renderingControl.setVerticalKeystone(id, std::stoi(xml::requiredChildValue(req, "DesiredVerticalKeystone")));
            break;
        case Action::GetMute:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            response.addArgument("CurrentMute", m_mute.at(id)[channelFromString(val)].getValue());
            break;
        }
        case Action::SetMute:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            m_renderingControl.setMute(id, channelFromString(val), xml::requiredChildValue(req, "DesiredMute") == "1");
            break;
        }
        case Action::GetVolume:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            response.addArgument("CurrentVolume", m_volumes.at(id)[channelFromString(val)].getValue());
            break;
        }
        case Action::SetVolume:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            m_renderingControl.setVolume(id, channelFromString(val), std::stoi(xml::requiredChildValue(req, "DesiredVolume")));
            break;
        }
        case Action::GetVolumeDB:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            response.addArgument("CurrentVolumeDB", m_dbVolumes.at(id)[channelFromString(val)].getValue());
            break;
        }
        case Action::SetVolumeDB:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            m_renderingControl.setVolumeDB(id, channelFromString(val), std::stoi(xml::requiredChildValue(req, "DesiredVolumeDB")));
            break;
        }
        case Action::GetVolumeDBRange:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            auto range = m_renderingControl.getVolumeDBRange(id, channelFromString(val));
            response.addArgument("MinValue", std::to_string(range.first));
            response.addArgument("MaxValue", std::to_string(range.second));
            break;
        }
        case Action::GetLoudness:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            response.addArgument("CurrentLoudness", m_loudness.at(id)[channelFromString(val)].getValue());
            break;
        }
        case Action::SetLoudness:
        {
            auto val = xml::requiredChildValue(req, "Channel");
            m_renderingControl.setLoudness(id, channelFromString(val), xml::requiredChildValue(req, "DesiredLoudness") == "1");
            break;
        }

        // RenderingControl:2
        //case Action::GetStateVariables:
        //    response.addArgument("StateVariableList", getStateVariables(id, req.getChildNodeValue("StateVariableList")).toString());
        //    break;
        //case Action::SetStateVariables:
        //    break;

        default:
            log::warn("No handler for RenderingControl action: {}", action);
            throw InvalidActionException();
        }

        return response;
    }
    catch (std::exception& e)
    {
        log::error("Error processing RenderingControl request: {}", e.what());
        throw InvalidActionException();
    }
}

void Service::setInstanceVariable(uint32_t id, Variable var, const std::string& value)
{
    DeviceService::setInstanceVariable(id, var, value);
    m_lastChange.addChangedVariable(id, ServiceVariable(toString(var), value));
}

const char* Service::variableToString(Variable type) const
{
    return RenderingControl::toString(type);
}

}
}
