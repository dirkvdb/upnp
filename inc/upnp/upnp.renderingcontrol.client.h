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

#include "utils/signal.h"

#include "upnp/upnpserviceclientbase.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpprotocolinfo.h"
#include "upnp/upnp.renderingcontrol.types.h"

#include <set>
#include <memory>

namespace upnp
{

class Action;
class IClient2;

namespace RenderingControl
{

struct ServiceTraits
{
    using ActionType = RenderingControl::Action;
    using VariableType = RenderingControl::Variable;
    static const ServiceType SvcType = ServiceType::RenderingControl;

    static ActionType actionFromString(const std::string& action);
    static const char* actionToString(ActionType action);
    static VariableType variableFromString(const std::string& var);
    static const char* variableToString(VariableType var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    Client(upnp::IClient2& client);

    void setVolume(int32_t connectionId, uint32_t value, std::function<void(int32_t status)> cb);
    void getVolume(int32_t connectionId, std::function<void(int32_t status, uint32_t volume)> cb);

    utils::Signal<const std::map<Variable, std::string>&> LastChangeEvent;

protected:
    virtual std::chrono::seconds getSubscriptionTimeout() override;

    virtual void processServiceDescription(const std::string& descriptionUrl, std::function<void(int32_t)> cb) override;

    virtual void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables) override;
    virtual void handleUPnPResult(int errorCode) override;

private:
    uint32_t                    m_minVolume;
    uint32_t                    m_maxVolume;
};

}
}
