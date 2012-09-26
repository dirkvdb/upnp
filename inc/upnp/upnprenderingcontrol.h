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

#include "upnp/upnpdevice.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpprotocolinfo.h"

#include <upnp/upnp.h>
#include <set>
#include <memory>

namespace upnp
{

class Action;
class Client;
    
class RenderingControl
{
public:
    enum class Action
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
    
    enum class Variable
    {
        PresetNameList,
        Mute,
        Volume,
        VolumeDB,
        LastChange //event
    };
    
    RenderingControl(Client& client);
    
    void setDevice(std::shared_ptr<Device> device);
    
    void subscribe();
    void unsubscribe();
    
    void increaseVolume(const std::string& connectionId, uint32_t percentage);
    void decreaseVolume(const std::string& connectionId, uint32_t percentage);
    void setVolume(const std::string& connectionId, int32_t value);
    
    utils::Signal<void(const std::map<Variable, std::string>&)> LastChangedEvent;
    
private:
    void parseServiceDescription(const std::string& descriptionUrl);
    void eventOccurred(Upnp_Event* pEvent);
    
    IXmlDocument executeAction(Action actionType, const std::string& connectionId, const std::map<std::string, std::string>& args = {});
    
    static int eventCb(Upnp_EventType eventType, void* pEvent, void* pInstance);
    static void handleUPnPResult(int errorCode);
    static Action actionFromString(const std::string& action);
    static std::string actionToString(Action action);
    static Variable variableFromString(const std::string& var);
    std::string variableToString(Variable var);

    Client&                     m_Client;
    Service                     m_Service;
    std::set<Action>            m_SupportedActions;
    
    uint32_t                    m_currentVolume;
    int32_t                     m_minVolume;
    int32_t                     m_maxVolume;
};

}

#endif
