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

#ifndef UPNP_RENDERING_CONTROL_SERVICE_H
#define UPNP_RENDERING_CONTROL_SERVICE_H

#include "upnp/upnpdeviceservice.h"

#include <string>
#include <vector>
#include <stdexcept>

namespace upnp
{

enum class Channel
{
    Master
};

class IRenderingControl
{
public:
    virtual ~IRenderingControl() {}
    
    // Required
    virtual std::vector<std::string> ListPresets() = 0;
    virtual void SelectPreset(const std::string& name) = 0;
    
    // Optional
    virtual uint16_t GetBrightness()                                        { throw InvalidActionException(); }
    virtual void SetBrightness(uint16_t value)                              { throw InvalidActionException(); }
    
    virtual uint16_t GetContrast()                                          { throw InvalidActionException(); }
    virtual void SetContrast(uint16_t value)                                { throw InvalidActionException(); }
    
    virtual uint16_t GetSharpness()                                         { throw InvalidActionException(); }
    virtual void SetSharpness(uint16_t value)                               { throw InvalidActionException(); }
    
    virtual uint16_t GetRedVideoGain()                                      { throw InvalidActionException(); }
    virtual void SetRedVideoGain(uint16_t value)                            { throw InvalidActionException(); }
    
    virtual uint16_t GetGreenVideoGain()                                    { throw InvalidActionException(); }
    virtual void SetGreenVideoGain(uint16_t value)                          { throw InvalidActionException(); }
    
    virtual uint16_t GetBlueVideoGain()                                     { throw InvalidActionException(); }
    virtual void SetBlueVideoGain(uint16_t value)                           { throw InvalidActionException(); }
    
    virtual uint16_t GetRedVideoBlackLevel()                                { throw InvalidActionException(); }
    virtual void SetRedVideoBlackLevel(uint16_t value)                      { throw InvalidActionException(); }
    
    virtual uint16_t GetGreenVideoBlackLevel()                              { throw InvalidActionException(); }
    virtual void SetGreenVideoBlackLevel(uint16_t value)                    { throw InvalidActionException(); }
    
    virtual uint16_t GetBlueVideoBlackLevel()                               { throw InvalidActionException(); }
    virtual void SetBlueVideoBlackLevel(uint16_t value)                     { throw InvalidActionException(); }
    
    virtual uint16_t GetColorTemperature()                                  { throw InvalidActionException(); }
    virtual void SetColorTemperature(uint16_t value)                        { throw InvalidActionException(); }
    
    virtual int16_t GetHorizontalKeystone()                                 { throw InvalidActionException(); }
    virtual void SetHorizontalKeystone(int16_t value)                       { throw InvalidActionException(); }
    
    virtual int16_t GetVerticalKeystone()                                   { throw InvalidActionException(); }
    virtual void SetVerticalKeystone(int16_t value)                         { throw InvalidActionException(); }
    
    virtual bool GetMute(Channel channel)                                   { throw InvalidActionException(); }
    virtual void SetMute(Channel channel, bool enabled)                     { throw InvalidActionException(); }
    
    virtual uint16_t GetVolume(Channel channel)                             { throw InvalidActionException(); }
    virtual void SetVolume(Channel channel, uint16_t value)                 { throw InvalidActionException(); }
    
    virtual int16_t SetVolumeDB(Channel channel)                            { throw InvalidActionException(); }
    virtual void GetVolumeDB(Channel channel, int16_t value)                { throw InvalidActionException(); }
    virtual std::pair<int16_t, int16_t> GetVolumeDBRange(Channel channel)   { throw InvalidActionException(); }
    
    virtual bool GetLoudness(Channel channel)                               { throw InvalidActionException(); }
    virtual void SetLoudness(Channel channel, bool enabled)                 { throw InvalidActionException(); }
};

class RenderingControlService : public DeviceService
{
public:
    RenderingControlService(IRenderingControl& rc);
    
    virtual ActionResponse onAction(const std::string& action, const xml::Document& request);
    
private:
    void listPresets(ActionResponse& response);

    IRenderingControl&  m_renderingControl;
};

}

#endif
