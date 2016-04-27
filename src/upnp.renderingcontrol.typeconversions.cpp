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

constexpr std::tuple<const char*, Action> s_actionNames[] {
    { "ListPresets",                Action::ListPresets },
    { "SelectPreset",               Action::SelectPreset },
    { "GetBrightness",              Action::GetBrightness },
    { "SetBrightness",              Action::SetBrightness },
    { "GetContrast",                Action::GetContrast },
    { "SetContrast",                Action::SetContrast },
    { "GetSharpness",               Action::GetSharpness },
    { "SetSharpness",               Action::SetSharpness },
    { "GetRedVideoGain",            Action::GetRedVideoGain },
    { "SetRedVideoGain",            Action::SetRedVideoGain },
    { "GetGreenVideoGain",          Action::GetGreenVideoGain },
    { "SetGreenVideoGain",          Action::SetGreenVideoGain },
    { "GetBlueVideoGain",           Action::GetBlueVideoGain },
    { "SetBlueVideoGain",           Action::SetBlueVideoGain },
    { "GetRedVideoBlackLevel",      Action::GetRedVideoBlackLevel },
    { "SetRedVideoBlackLevel",      Action::SetRedVideoBlackLevel },
    { "GetGreenVideoBlackLevel",    Action::GetGreenVideoBlackLevel },
    { "SetGreenVideoBlackLevel",    Action::SetGreenVideoBlackLevel },
    { "GetBlueVideoBlackLevel",     Action::GetBlueVideoBlackLevel },
    { "SetBlueVideoBlackLevel",     Action::SetBlueVideoBlackLevel },
    { "GetColorTemperature",        Action::GetColorTemperature },
    { "SetColorTemperature",        Action::SetColorTemperature },
    { "GetHorizontalKeystone",      Action::GetHorizontalKeystone },
    { "SetHorizontalKeystone",      Action::SetHorizontalKeystone },
    { "GetVerticalKeystone",        Action::GetVerticalKeystone },
    { "SetVerticalKeystone",        Action::SetVerticalKeystone },
    { "GetMute",                    Action::GetMute },
    { "SetMute",                    Action::SetMute },
    { "GetVolume",                  Action::GetVolume },
    { "SetVolume",                  Action::SetVolume },
    { "GetVolumeDB",                Action::GetVolumeDB },
    { "SetVolumeDB",                Action::SetVolumeDB },
    { "GetVolumeDBRange",           Action::GetVolumeDBRange },
    { "GetLoudness",                Action::GetLoudness },
    { "SetLoudness",                Action::SetLoudness },

    { "GetStateVariables",          Action::GetStateVariables },
    { "SetStateVariables",          Action::SetStateVariables },
};

constexpr std::tuple<const char*, Variable> s_variableNames[] {
    { "PresetNameList",                        Variable::PresetNameList },
    { "Brightness",                            Variable::Brightness },
    { "Contrast",                              Variable::Contrast },
    { "Sharpness",                             Variable::Sharpness },
    { "RedVideoGain",                          Variable::RedVideoGain },
    { "GreenVideoGain",                        Variable::GreenVideoGain },
    { "BlueVideoGain",                         Variable::BlueVideoGain },
    { "RedVideoBlackLevel",                    Variable::RedVideoBlackLevel },
    { "GreenVideoBlackLevel",                  Variable::GreenVideoBlackLevel },
    { "BlueVideoBlackLevel",                   Variable::BlueVideoBlackLevel },
    { "ColorTemperature",                      Variable::ColorTemperature },
    { "HorizontalKeystone",                    Variable::HorizontalKeystone },
    { "VerticalKeystone",                      Variable::VerticalKeystone },
    { "Mute",                                  Variable::Mute },
    { "Volume",                                Variable::Volume },
    { "VolumeDB",                              Variable::VolumeDB },
    { "Loudness",                              Variable::Loudness },
    { "LastChange",                            Variable::LastChange },

    { "A_ARG_TYPE_DeviceUDN",                  Variable::ArgumentTypeDeviceUDN },
    { "A_ARG_TYPE_ServiceType",                Variable::ArgumentTypeServiceType },
    { "A_ARG_TYPE_ServiceId",                  Variable::ArgumentTypeServiceId },
    { "A_ARG_TYPE_StateVariableValuePairs",    Variable::ArgumentTypeStateVariableValuePairs },
    { "A_ARG_TYPE_StateVariableList",          Variable::ArgumentTypeStateVariableList },
};

constexpr std::tuple<const char*, Channel> s_channelNames[] {
    { "Master",     Channel::Master },
    { "LF",         Channel::LF },
    { "RF",         Channel::RF },
    { "CF",         Channel::CF },
    { "LFE",        Channel::LFE },
    { "LS",         Channel::LS },
    { "RS",         Channel::RS },
    { "LFC",        Channel::LFC },
    { "RFC",        Channel::RFC },
    { "SD",         Channel::SD },
    { "SL",         Channel::SL },
    { "SR",         Channel::SR },
    { "T",          Channel::T },
    { "B",          Channel::B },
};

template<> constexpr const std::tuple<const char*, Action>* lut<Action>() { return s_actionNames; }
template<> constexpr const std::tuple<const char*, Variable>* lut<Variable>() { return s_variableNames; }
template<> constexpr const std::tuple<const char*, Channel>* lut<Channel>() { return s_channelNames; }

static_assert(enumCorrectNess<Action>(), "Action enum converion not correctly ordered or missing entries");
static_assert(enumCorrectNess<Variable>(), "Action enum converion not correctly ordered or missing entries");
static_assert(enumCorrectNess<Channel>(), "Channel enum converion not correctly ordered or missing entries");

Action RenderingControl::actionFromString(gsl::span<const char> value)
{
    return fromString<Action>(value.data(), value.size());
}

const char* RenderingControl::toString(Action value)
{
    return upnp::toString(value);
}

Variable RenderingControl::variableFromString(gsl::span<const char> value)
{
    return fromString<Variable>(value.data(), value.size());
}

const char* RenderingControl::toString(Variable value)
{
    return upnp::toString(value);
}

Channel RenderingControl::channelFromString(gsl::span<const char> value)
{
    return fromString<Channel>(value.data(), value.size());
}

const char* RenderingControl::toString(Channel value)
{
    return upnp::toString(value);
}

}
