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

#include "upnp.renderingcontrol.typeconversions.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace RenderingControl;

static constexpr EnumMap<Action> s_actionNames {{
    std::make_tuple("ListPresets",                Action::ListPresets),
    std::make_tuple("SelectPreset",               Action::SelectPreset),
    std::make_tuple("GetBrightness",              Action::GetBrightness),
    std::make_tuple("SetBrightness",              Action::SetBrightness),
    std::make_tuple("GetContrast",                Action::GetContrast),
    std::make_tuple("SetContrast",                Action::SetContrast),
    std::make_tuple("GetSharpness",               Action::GetSharpness),
    std::make_tuple("SetSharpness",               Action::SetSharpness),
    std::make_tuple("GetRedVideoGain",            Action::GetRedVideoGain),
    std::make_tuple("SetRedVideoGain",            Action::SetRedVideoGain),
    std::make_tuple("GetGreenVideoGain",          Action::GetGreenVideoGain),
    std::make_tuple("SetGreenVideoGain",          Action::SetGreenVideoGain),
    std::make_tuple("GetBlueVideoGain",           Action::GetBlueVideoGain),
    std::make_tuple("SetBlueVideoGain",           Action::SetBlueVideoGain),
    std::make_tuple("GetRedVideoBlackLevel",      Action::GetRedVideoBlackLevel),
    std::make_tuple("SetRedVideoBlackLevel",      Action::SetRedVideoBlackLevel),
    std::make_tuple("GetGreenVideoBlackLevel",    Action::GetGreenVideoBlackLevel),
    std::make_tuple("SetGreenVideoBlackLevel",    Action::SetGreenVideoBlackLevel),
    std::make_tuple("GetBlueVideoBlackLevel",     Action::GetBlueVideoBlackLevel),
    std::make_tuple("SetBlueVideoBlackLevel",     Action::SetBlueVideoBlackLevel),
    std::make_tuple("GetColorTemperature",        Action::GetColorTemperature),
    std::make_tuple("SetColorTemperature",        Action::SetColorTemperature),
    std::make_tuple("GetHorizontalKeystone",      Action::GetHorizontalKeystone),
    std::make_tuple("SetHorizontalKeystone",      Action::SetHorizontalKeystone),
    std::make_tuple("GetVerticalKeystone",        Action::GetVerticalKeystone),
    std::make_tuple("SetVerticalKeystone",        Action::SetVerticalKeystone),
    std::make_tuple("GetMute",                    Action::GetMute),
    std::make_tuple("SetMute",                    Action::SetMute),
    std::make_tuple("GetVolume",                  Action::GetVolume),
    std::make_tuple("SetVolume",                  Action::SetVolume),
    std::make_tuple("GetVolumeDB",                Action::GetVolumeDB),
    std::make_tuple("SetVolumeDB",                Action::SetVolumeDB),
    std::make_tuple("GetVolumeDBRange",           Action::GetVolumeDBRange),
    std::make_tuple("GetLoudness",                Action::GetLoudness),
    std::make_tuple("SetLoudness",                Action::SetLoudness),

    std::make_tuple("GetStateVariables",          Action::GetStateVariables),
    std::make_tuple("SetStateVariables",          Action::SetStateVariables),
}};

static constexpr EnumMap<Variable> s_variableNames {{
    std::make_tuple("PresetNameList",                        Variable::PresetNameList),
    std::make_tuple("Brightness",                            Variable::Brightness),
    std::make_tuple("Contrast",                              Variable::Contrast),
    std::make_tuple("Sharpness",                             Variable::Sharpness),
    std::make_tuple("RedVideoGain",                          Variable::RedVideoGain),
    std::make_tuple("GreenVideoGain",                        Variable::GreenVideoGain),
    std::make_tuple("BlueVideoGain",                         Variable::BlueVideoGain),
    std::make_tuple("RedVideoBlackLevel",                    Variable::RedVideoBlackLevel),
    std::make_tuple("GreenVideoBlackLevel",                  Variable::GreenVideoBlackLevel),
    std::make_tuple("BlueVideoBlackLevel",                   Variable::BlueVideoBlackLevel),
    std::make_tuple("ColorTemperature",                      Variable::ColorTemperature),
    std::make_tuple("HorizontalKeystone",                    Variable::HorizontalKeystone),
    std::make_tuple("VerticalKeystone",                      Variable::VerticalKeystone),
    std::make_tuple("Mute",                                  Variable::Mute),
    std::make_tuple("Volume",                                Variable::Volume),
    std::make_tuple("VolumeDB",                              Variable::VolumeDB),
    std::make_tuple("Loudness",                              Variable::Loudness),
    std::make_tuple("LastChange",                            Variable::LastChange),

    std::make_tuple("A_ARG_TYPE_DeviceUDN",                  Variable::ArgumentTypeDeviceUDN),
    std::make_tuple("A_ARG_TYPE_ServiceType",                Variable::ArgumentTypeServiceType),
    std::make_tuple("A_ARG_TYPE_ServiceId",                  Variable::ArgumentTypeServiceId),
    std::make_tuple("A_ARG_TYPE_StateVariableValuePairs",    Variable::ArgumentTypeStateVariableValuePairs),
    std::make_tuple("A_ARG_TYPE_StateVariableList",          Variable::ArgumentTypeStateVariableList),
}};

static constexpr EnumMapEndsWith<Channel, Channel::B> s_channelNames {{
    std::make_tuple("Master",     Channel::Master),
    std::make_tuple("LF",         Channel::LF),
    std::make_tuple("RF",         Channel::RF),
    std::make_tuple("CF",         Channel::CF),
    std::make_tuple("LFE",        Channel::LFE),
    std::make_tuple("LS",         Channel::LS),
    std::make_tuple("RS",         Channel::RS),
    std::make_tuple("LFC",        Channel::LFC),
    std::make_tuple("RFC",        Channel::RFC),
    std::make_tuple("SD",         Channel::SD),
    std::make_tuple("SL",         Channel::SL),
    std::make_tuple("SR",         Channel::SR),
    std::make_tuple("T",          Channel::T),
    std::make_tuple("B",          Channel::B),
}};

ADD_ENUM_MAP(Action, s_actionNames)
ADD_ENUM_MAP(Variable, s_variableNames)
ADD_ENUM_MAP(Channel, s_channelNames)

Action RenderingControl::actionFromString(std::string_view value)
{
    return enum_cast<Action>(value);
}

const char* RenderingControl::toString(Action value)
{
    return enum_string(value);
}

Variable RenderingControl::variableFromString(std::string_view value)
{
    return enum_cast<Variable>(value);
}

const char* RenderingControl::toString(Variable value)
{
    return enum_string(value);
}

Channel RenderingControl::channelFromString(std::string_view value)
{
    return enum_cast<Channel>(value);
}

const char* RenderingControl::toString(Channel value)
{
    return enum_string(value);
}

}
