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
#include "upnp/upnplastchangevariable.h"
#include "upnp/upnp.renderingcontrol.types.h"

#include <string>
#include <vector>
#include <stdexcept>

namespace upnp
{

class IRenderingControl
{
public:
    virtual ~IRenderingControl() {}

    // Required
    virtual void selectPreset(uint32_t instanceId, const std::string& name) = 0;

    // Optional
    virtual void setBrightness(uint32_t /*instanceId*/, uint16_t)                                                           { throw InvalidActionException(); }
    virtual void setContrast(uint32_t /*instanceId*/, uint16_t)                                                             { throw InvalidActionException(); }
    virtual void setSharpness(uint32_t /*instanceId*/, uint16_t)                                                            { throw InvalidActionException(); }
    virtual void setRedVideoGain(uint32_t /*instanceId*/, uint16_t)                                                         { throw InvalidActionException(); }
    virtual void setGreenVideoGain(uint32_t /*instanceId*/, uint16_t)                                                       { throw InvalidActionException(); }
    virtual void setBlueVideoGain(uint32_t /*instanceId*/, uint16_t)                                                        { throw InvalidActionException(); }
    virtual void setRedVideoBlackLevel(uint32_t /*instanceId*/, uint16_t)                                                   { throw InvalidActionException(); }
    virtual void setGreenVideoBlackLevel(uint32_t /*instanceId*/, uint16_t)                                                 { throw InvalidActionException(); }
    virtual void setBlueVideoBlackLevel(uint32_t /*instanceId*/, uint16_t)                                                  { throw InvalidActionException(); }
    virtual void setColorTemperature(uint32_t /*instanceId*/, uint16_t)                                                     { throw InvalidActionException(); }
    virtual void setHorizontalKeystone(uint32_t /*instanceId*/, int16_t)                                                    { throw InvalidActionException(); }
    virtual void setVerticalKeystone(uint32_t /*instanceId*/, int16_t)                                                      { throw InvalidActionException(); }
    virtual void setMute(uint32_t /*instanceId*/, RenderingControl::Channel /*channel*/, bool /*enabled*/)                  { throw InvalidActionException(); }
    virtual void setVolume(uint32_t /*instanceId*/, RenderingControl::Channel /*channel*/, uint16_t /*value*/)              { throw InvalidActionException(); }
    virtual int16_t setVolumeDB(uint32_t /*instanceId*/, RenderingControl::Channel /*channel*/, int16_t /*value*/)          { throw InvalidActionException(); }
    virtual std::pair<int16_t, int16_t> getVolumeDBRange(uint32_t /*instanceId*/, RenderingControl::Channel /*channel*/)    { throw InvalidActionException(); }
    virtual void setLoudness(uint32_t /*instanceId*/, RenderingControl::Channel /*channel*/, bool /*enabled*/)              { throw InvalidActionException(); }
};

namespace RenderingControl
{

class Service : public DeviceService<Variable>
{
public:
    Service(IRootDevice& dev, IRenderingControl& rc);
    ~Service();

    void setMute(uint32_t instanceId, Channel channel, bool enabled);
    void setLoudness(uint32_t instanceId, Channel channel, bool enabled);
    void setVolume(uint32_t instanceId, Channel channel, uint16_t volume);
    void setVolumeDB(uint32_t instanceId, Channel channel, int16_t volume);

    virtual std::string getSubscriptionResponse() override;
    virtual ActionResponse onAction(const std::string& action, const xml::Document& request) override;

    virtual void setInstanceVariable(uint32_t id, Variable var, const std::string& value) override;

protected:
    virtual const char* variableToString(Variable type) const override;

private:
    void updateAudioVariable(std::map<uint32_t, std::map<Channel, ServiceVariable>>& vars, uint32_t instanceId, Channel channel, Variable var, const std::string& value);

    IRenderingControl&                                       m_renderingControl;
    LastChangeVariable                                       m_lastChange;

    // variables which are mapped per channel
    std::map<uint32_t, std::map<Channel, ServiceVariable>>   m_mute;
    std::map<uint32_t, std::map<Channel, ServiceVariable>>   m_loudness;
    std::map<uint32_t, std::map<Channel, ServiceVariable>>   m_volumes;
    std::map<uint32_t, std::map<Channel, ServiceVariable>>   m_dbVolumes;
};

}
}

#endif
