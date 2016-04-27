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

#pragma once

namespace upnp
{
namespace RenderingControl
{

enum class Action
{
// RenderingControl:1
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
    SetLoudness,

// RenderingControl:2
    GetStateVariables, // Optional
    SetStateVariables, // Optional

    EnumCount
};

enum class Variable
{
// RenderingControl:1
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
    LastChange, //event

// RenderingControl:2
    ArgumentTypeDeviceUDN, // Optional
    ArgumentTypeServiceType, // Optional
    ArgumentTypeServiceId, // Optional
    ArgumentTypeStateVariableValuePairs, // Optional
    ArgumentTypeStateVariableList, // Optional

    EnumCount
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
    B,

    EnumCount
};

}
}
