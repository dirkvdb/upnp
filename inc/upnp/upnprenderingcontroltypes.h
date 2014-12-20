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

#ifndef UPNP_RENDERING_CONTROL_TYPES_H
#define UPNP_RENDERING_CONTROL_TYPES_H

namespace upnp
{
namespace RenderingControl
{

enum class Action
{
    ListPresets,    // Required
    SelectPreset,   // Required
    GetBrightness,
    SetBrightness,
    GetContrast,
    SetContrast,
    GetSharpness,
    SetSharpness,
    GetRedVideoGain,
    SetRedVideoGain,
    GetGreenVideoGain,
    SetGreenVideoGain,
    GetBlueVideoGain,
    SetBlueVideoGain,
    GetRedVideoBlackLevel,
    SetRedVideoBlackLevel,
    GetGreenVideoBlackLevel,
    SetGreenVideoBlackLevel,
    GetBlueVideoBlackLevel,
    SetBlueVideoBlackLevel,
    GetColorTemperature,
    SetColorTemperature,
    GetHorizontalKeystone,
    SetHorizontalKeystone,
    GetVerticalKeystone,
    SetVerticalKeystone,
    GetMute,
    SetMute,
    GetVolume,
    SetVolume,
    GetVolumeDB,
    SetVolumeDB,
    GetVolumeDBRange,
    GetLoudness,
    SetLoudness
};

enum class Variable
{
    PresetNameList,
    Brightness,
    Contrast,
    Sharpness,
    RedVideoGain,
    GreenVideoGain,
    BlueVideoGain,
    RedVideoBlackLevel,
    GreenVideoBlackLevel,
    BlueVideoBlackLevel,
    ColorTemperature,
    HorizontalKeystone,
    VerticalKeystone,
    Mute,
    Volume,
    VolumeDB,
    Loudness,
    LastChange //event
};

enum class Channel
{
    Master, // Required
    LF,
    RF,
    CF,
    LFE,
    LS,
    RS,
    LFC,
    RFC,
    SD,
    SL,
    SR,
    T,
    B
};

inline Action actionFromString(const std::string& action)
{
    if (action == "ListPresets")                return Action::ListPresets;
    if (action == "SelectPreset")               return Action::SelectPreset;
    if (action == "GetBrightness")              return Action::GetBrightness;
    if (action == "SetBrightness")              return Action::SetBrightness;
    if (action == "GetContrast")                return Action::GetContrast;
    if (action == "SetContrast")                return Action::SetContrast;
    if (action == "GetSharpness")               return Action::GetSharpness;
    if (action == "SetSharpness")               return Action::SetSharpness;
    if (action == "GetRedVideoGain")            return Action::GetRedVideoGain;
    if (action == "SetRedVideoGain")            return Action::SetRedVideoGain;
    if (action == "GetGreenVideoGain")          return Action::GetGreenVideoGain;
    if (action == "SetGreenVideoGain")          return Action::SetGreenVideoGain;
    if (action == "GetBlueVideoGain")           return Action::GetBlueVideoGain;
    if (action == "SetBlueVideoGain")           return Action::SetBlueVideoGain;
    if (action == "GetRedVideoBlackLevel")      return Action::GetRedVideoBlackLevel;
    if (action == "SetRedVideoBlackLevel")      return Action::SetRedVideoBlackLevel;
    if (action == "GetGreenVideoBlackLevel")    return Action::GetGreenVideoBlackLevel;
    if (action == "SetGreenVideoBlackLevel")    return Action::SetGreenVideoBlackLevel;
    if (action == "GetBlueVideoBlackLevel")     return Action::GetBlueVideoBlackLevel;
    if (action == "SetBlueVideoBlackLevel")     return Action::SetBlueVideoBlackLevel;
    if (action == "GetColorTemperature")        return Action::GetColorTemperature;
    if (action == "SetColorTemperature")        return Action::SetColorTemperature;
    if (action == "GetHorizontalKeystone")      return Action::GetHorizontalKeystone;
    if (action == "SetHorizontalKeystone")      return Action::SetHorizontalKeystone;
    if (action == "GetVerticalKeystone")        return Action::GetVerticalKeystone;
    if (action == "SetVerticalKeystone")        return Action::SetVerticalKeystone;
    if (action == "GetVolume")                  return Action::GetVolume;
    if (action == "SetVolume")                  return Action::SetVolume;
    if (action == "GetVolumeDB")                return Action::GetVolumeDB;
    if (action == "GetVolumeDBRange")           return Action::GetVolumeDBRange;
    if (action == "SetVolumeDB")                return Action::SetVolumeDB;
    if (action == "GetMute")                    return Action::GetMute;
    if (action == "SetMute")                    return Action::SetMute;
    if (action == "GetLoudness")                return Action::GetLoudness;
    if (action == "SetLoudness")                return Action::SetLoudness;
    
    throw Exception("Unknown RendereringControl action: {}", action);
}

inline std::string toString(Action action)
{
    switch (action)
    {
    case Action::ListPresets:                   return "ListPresets";
    case Action::SelectPreset:                  return "SelectPreset";
    case Action::GetBrightness:                 return "GetBrightness";
    case Action::SetBrightness:                 return "SetBrightness";
    case Action::GetContrast:                   return "GetContrast";
    case Action::SetContrast:                   return "SetContrast";
    case Action::GetSharpness:                  return "GetSharpness";
    case Action::SetSharpness:                  return "SetSharpness";
    case Action::GetRedVideoGain:               return "GetRedVideoGain";
    case Action::SetRedVideoGain:               return "SetRedVideoGain";
    case Action::GetGreenVideoGain:             return "GetGreenVideoGain";
    case Action::SetGreenVideoGain:             return "SetGreenVideoGain";
    case Action::GetBlueVideoGain:              return "GetBlueVideoGain";
    case Action::SetBlueVideoGain:              return "SetBlueVideoGain";
    case Action::GetRedVideoBlackLevel:         return "GetRedVideoBlackLevel";
    case Action::SetRedVideoBlackLevel:         return "SetRedVideoBlackLevel";
    case Action::GetGreenVideoBlackLevel:       return "GetGreenVideoBlackLevel";
    case Action::SetGreenVideoBlackLevel:       return "SetGreenVideoBlackLevel";
    case Action::GetBlueVideoBlackLevel:        return "GetBlueVideoBlackLevel";
    case Action::SetBlueVideoBlackLevel:        return "SetBlueVideoBlackLevel";
    case Action::GetColorTemperature:           return "GetColorTemperature";
    case Action::SetColorTemperature:           return "SetColorTemperature";
    case Action::GetHorizontalKeystone:         return "GetHorizontalKeystone";
    case Action::SetHorizontalKeystone:         return "SetHorizontalKeystone";
    case Action::GetVerticalKeystone:           return "GetVerticalKeystone";
    case Action::SetVerticalKeystone:           return "SetVerticalKeystone";
    case Action::GetMute:                       return "GetMute";
    case Action::SetMute:                       return "SetMute";
    case Action::GetVolume:                     return "GetVolume";
    case Action::SetVolume:                     return "SetVolume";
    case Action::GetVolumeDB:                   return "GetVolumeDB";
    case Action::GetVolumeDBRange:              return "GetVolumeDBRange";
    case Action::SetVolumeDB:                   return "SetVolumeDB";
    case Action::SetLoudness:                   return "SetLoudness";
    case Action::GetLoudness:                   return "GetLoudness";
    default:
        throw Exception("Invalid RenderingControl action: {}", static_cast<int32_t>(action));
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "PresetNameList")                    return Variable::PresetNameList;
    if (var == "Brightness")                        return Variable::Brightness;
    if (var == "Contrast")                          return Variable::Contrast;
    if (var == "Sharpness")                         return Variable::Sharpness;
    if (var == "RedVideoGain")                      return Variable::RedVideoGain;
    if (var == "GreenVideoGain")                    return Variable::GreenVideoGain;
    if (var == "BlueVideoGain")                     return Variable::BlueVideoGain;
    if (var == "RedVideoBlackLevel")                return Variable::RedVideoBlackLevel;
    if (var == "GreenVideoBlackLevel")              return Variable::GreenVideoBlackLevel;
    if (var == "BlueVideoBlackLevel")               return Variable::BlueVideoBlackLevel;
    if (var == "ColorTemperature")                  return Variable::ColorTemperature;
    if (var == "HorizontalKeystone")                return Variable::HorizontalKeystone;
    if (var == "VerticalKeystone")                  return Variable::VerticalKeystone;
    if (var == "Mute")                              return Variable::Mute;
    if (var == "Volume")                            return Variable::Volume;
    if (var == "VolumeDB")                          return Variable::VolumeDB;
    if (var == "Loudness")                          return Variable::Loudness;
    if (var == "LastChange")                        return Variable::LastChange;
    
    throw Exception("Unknown RenderingControl variable: {}", var);
}

inline std::string toString(Variable var)
{
    switch (var)
    {
        case Variable::PresetNameList:              return "PresetNameList";
        case Variable::Brightness:                  return "Brightness";
        case Variable::Contrast:                    return "Contrast";
        case Variable::Sharpness:                   return "Sharpness";
        case Variable::RedVideoGain:                return "RedVideoGain";
        case Variable::GreenVideoGain:              return "GreenVideoGain";
        case Variable::BlueVideoGain:               return "BlueVideoGain";
        case Variable::RedVideoBlackLevel:          return "RedVideoBlackLevel";
        case Variable::GreenVideoBlackLevel:        return "GreenVideoBlackLevel";
        case Variable::BlueVideoBlackLevel:         return "BlueVideoBlackLevel";
        case Variable::ColorTemperature:            return "ColorTemperature";
        case Variable::HorizontalKeystone:          return "HorizontalKeystone";
        case Variable::VerticalKeystone:            return "VerticalKeystone";
        case Variable::Mute:                        return "Mute";
        case Variable::Volume:                      return "Volume";
        case Variable::VolumeDB:                    return "VolumeDB";
        case Variable::Loudness:                    return "Loudness";
        case Variable::LastChange:                  return "LastChange";
        default:
            throw Exception("Invalid RenderingControl variable: {}", static_cast<int32_t>(var));
    }
}

inline Channel channelFromString(const std::string& channel)
{
    if (channel == "Master")            return Channel::Master;
    if (channel == "LF")                return Channel::LF;
    if (channel == "RF")                return Channel::RF;
    if (channel == "CF")                return Channel::CF;
    if (channel == "LFE")               return Channel::LFE;
    if (channel == "LS")                return Channel::LS;
    if (channel == "RS")                return Channel::RS;
    if (channel == "RFC")               return Channel::RFC;
    if (channel == "SD")                return Channel::SD;
    if (channel == "SL")                return Channel::SL;
    if (channel == "SR")                return Channel::SR;
    if (channel == "T")                 return Channel::T;
    if (channel == "B")                 return Channel::B;
    
    throw Exception("Unknown RendereringControl channel: {}", channel);
}

inline std::string toString(Channel channel)
{
    switch (channel)
    {
    case Channel::Master:               return "Master";
    case Channel::LF:                   return "LF";
    case Channel::RF:                   return "RF";
    case Channel::CF:                   return "CF";
    case Channel::LFE:                  return "LFE";
    case Channel::LS:                   return "LS";
    case Channel::RS:                   return "RS";
    case Channel::RFC:                  return "RFC";
    case Channel::SD:                   return "SD";
    case Channel::SL:                   return "SL";
    case Channel::SR:                   return "SR";
    case Channel::T:                    return "T";
    case Channel::B:                    return "B";
    default:
        throw Exception("Invalid RenderingControl channel: {}", static_cast<int32_t>(channel));
    }
}
    
}
}

#endif
