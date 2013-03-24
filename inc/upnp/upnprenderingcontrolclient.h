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

#ifndef UPNP_RENDERING_CONTROL_CLIENT_H
#define UPNP_RENDERING_CONTROL_CLIENT_H

#include "utils/signal.h"

#include "upnp/upnpservicebase.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnprenderingcontroltypes.h"

#include <upnp/upnp.h>
#include <set>
#include <memory>

namespace upnp
{

class Action;
class IClient;

namespace RenderingControl
{

class Client : public ServiceBase<Action, Variable>
{
public:
    Client(IClient& client);
    
    void setVolume(int32_t connectionId, uint32_t value);
    uint32_t getVolume(int32_t connectionId);
    
    utils::Signal<void(const std::map<Variable, std::string>&)> LastChangeEvent;
    
protected:
    virtual Action actionFromString(const std::string& action);
    virtual std::string actionToString(Action action);
    virtual Variable variableFromString(const std::string& var);
    virtual std::string variableToString(Variable var);

    virtual ServiceType getType();
    virtual int32_t getSubscriptionTimeout();
    
    virtual void parseServiceDescription(const std::string& descriptionUrl);

    virtual void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables);
    virtual void handleUPnPResult(int errorCode);

private:
    uint32_t                    m_MinVolume;
    uint32_t                    m_MaxVolume;
};

}
}

#endif
