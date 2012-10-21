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

#ifndef UPNP_RENDERING_CONTROL_H
#define UPNP_RENDERING_CONTROL_H

#include "utils/signal.h"

#include "upnp/upnpservicebase.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpprotocolinfo.h"

#include <upnp/upnp.h>
#include <set>
#include <memory>

namespace upnp
{

class Action;
class IClient;

enum class RenderingControlAction
{
    ListPresets,
    SelectPreset,
    GetVolume,
    SetVolume,
    GetVolumeDB,
    SetVolumeDB,
    GetMute,
    SetMute
};

enum class RenderingControlVariable
{
    PresetNameList,
    Mute,
    Volume,
    VolumeDB,
    LastChange //event
};
    
class RenderingControl : public ServiceBase<RenderingControlAction, RenderingControlVariable>
{
public:
    RenderingControl(IClient& client);
    
    void setVolume(const std::string& connectionId, uint32_t value);
    uint32_t getVolume(const std::string& connectionId);
    
    utils::Signal<void(const std::map<RenderingControlVariable, std::string>&)> LastChangeEvent;
    
protected:
    virtual ServiceType getType();
    virtual int32_t getSubscriptionTimeout();
    
    virtual void parseServiceDescription(const std::string& descriptionUrl);

    virtual void handleStateVariableEvent(RenderingControlVariable var, const std::map<RenderingControlVariable, std::string>& variables);
    virtual void handleUPnPResult(int errorCode);
    virtual RenderingControlAction actionFromString(const std::string& action);
    virtual std::string actionToString(RenderingControlAction action);
    virtual RenderingControlVariable variableFromString(const std::string& var);
    virtual std::string variableToString(RenderingControlVariable var);

private:
    uint32_t                    m_MinVolume;
    uint32_t                    m_MaxVolume;
};

}

#endif
