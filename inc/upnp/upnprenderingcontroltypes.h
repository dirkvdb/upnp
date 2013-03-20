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
    ListPresets,
    SelectPreset,
    GetVolume,
    SetVolume,
    GetVolumeDB,
    GetVolumeDBRange,
    SetVolumeDB,
    GetMute,
    SetMute
};

enum class Variable
{
    PresetNameList,
    Mute,
    Volume,
    VolumeDB,
    LastChange //event
};

inline Action actionFromString(const std::string& action)
{
    if (action == "ListPresets")                return Action::ListPresets;
    if (action == "SelectPreset")               return Action::SelectPreset;
    if (action == "GetVolume")                  return Action::GetVolume;
    if (action == "SetVolume")                  return Action::SetVolume;
    if (action == "GetVolumeDB")                return Action::GetVolumeDB;
    if (action == "GetVolumeDBRange")           return Action::GetVolumeDBRange;
    if (action == "SetVolumeDB")                return Action::SetVolumeDB;
    if (action == "GetMute")                    return Action::GetMute;
    if (action == "SetMute")                    return Action::SetMute;
    
    throw std::logic_error("Unknown RendereringControl action:" + action);
}

inline std::string actionToString(Action action)
{
    switch (action)
    {
    case Action::ListPresets:           return "ListPresets";
    case Action::SelectPreset:          return "SelectPreset";
    case Action::GetVolume:             return "GetVolume";
    case Action::SetVolume:             return "SetVolume";
    case Action::GetVolumeDB:           return "GetVolumeDB";
    case Action::GetVolumeDBRange:      return "GetVolumeDBRange";
    case Action::SetVolumeDB:           return "SetVolumeDB";
    case Action::GetMute:               return "GetMute";
    case Action::SetMute:               return "SetMute";
    default:
        throw std::logic_error("Invalid RenderingControl action");
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "PresetNameList")                    return Variable::PresetNameList;
    if (var == "Mute")                              return Variable::Mute;
    if (var == "Volume")                            return Variable::Volume;
    if (var == "VolumeDB")                          return Variable::VolumeDB;
    if (var == "LastChange")                        return Variable::LastChange;
    
    throw std::logic_error("Unknown RenderingControl variable:" + var);
}

inline std::string variableToString(Variable var)
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
}

#endif
