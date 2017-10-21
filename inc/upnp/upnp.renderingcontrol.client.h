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

#include "upnp/upnp.serviceclientbase.h"
#include "upnp/upnp.device.h"
#include "upnp/upnp.protocolinfo.h"
#include "upnp/upnp.renderingcontrol.types.h"

#include <set>
#include <memory>

namespace upnp
{

class Action;
class IClient;

namespace RenderingControl
{

struct ServiceTraits
{
    using ActionType = RenderingControl::Action;
    using VariableType = RenderingControl::Variable;
    static ServiceType::Type SvcType;
    static constexpr uint8_t SvcVersion = 1;

    static ActionType actionFromString(const std::string& action);
    static const char* actionToString(ActionType action);
    static VariableType variableFromString(const std::string& var);
    static const char* variableToString(VariableType var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    Client(upnp::IClient& client);

    void setVolume(int32_t connectionId, uint32_t value, std::function<void(Status status)> cb);
    void getVolume(int32_t connectionId, std::function<void(Status status, uint32_t volume)> cb);

    Future<void> setVolume(int32_t connectionId, uint32_t value);
    Future<uint32_t> getVolume(int32_t connectionId);

    utils::Signal<const std::map<Variable, std::string>&> LastChangeEvent;

protected:
    virtual std::chrono::seconds getSubscriptionTimeout() override;

    Future<void> processServiceDescription(const std::string& descriptionUrl) override;
    void processServiceDescription(const std::string& descriptionUrl, std::function<void(Status)> cb) override;

    void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables) override;

private:
    uint32_t                    m_minVolume;
    uint32_t                    m_maxVolume;
};

}
}
