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

#include "upnp/upnprenderingcontrolclient.h"

#include "upnp/upnputils.h"
#include "upnp/upnpclientinterface.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

using namespace utils;
using namespace std::placeholders;

namespace upnp
{
namespace RenderingControl
{

static const int32_t g_subscriptionTimeout = 1801;

// TODO: assign min and max volume

Client::Client(IClient& client)
: ServiceClientBase(client)
, m_MinVolume(0)
, m_MaxVolume(100)
{
}

void Client::setVolume(int32_t connectionId, uint32_t value)
{
    numericops::clip(value, m_MinVolume, m_MaxVolume);
    executeAction(Action::SetVolume, { {"InstanceID", std::to_string(connectionId)},
                                       {"Channel", "Master"},
                                       {"DesiredVolume", numericops::toString(value)} });
}

uint32_t Client::getVolume(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetVolume, { {"InstanceID", std::to_string(connectionId)},
                                                           {"Channel", "Master"} });
    
    xml::Element response = doc.getFirstChild();
    return stringops::toNumeric<uint32_t>(response.getChildNodeValue("CurrentVolume"));
}

ServiceType Client::getType()
{
    return ServiceType::RenderingControl;
}

int32_t Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

void Client::parseServiceDescription(const std::string& descriptionUrl)
{
    ServiceClientBase::parseServiceDescription(descriptionUrl);
    
    for (auto& variable : m_StateVariables)
    {
        if (variable.name == variableToString(Variable::Volume))
        {
            if (variable.valueRange)
            {
                m_MinVolume = variable.valueRange->minimumValue;
                m_MaxVolume = variable.valueRange->maximumValue;
            }
        }
    }
}

void Client::handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables)
{
    if (var == Variable::LastChange)
    {
        LastChangeEvent(variables);
    }
}

void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 702: throw std::logic_error("Invalid instance id");
        default: upnp::handleUPnPResult(errorCode);
    }
}

Action Client::actionFromString(const std::string& action) const
{
    return RenderingControl::actionFromString(action);
}

std::string Client::actionToString(Action action) const
{
    return RenderingControl::toString(action);
}

Variable Client::variableFromString(const std::string& var) const
{
    return RenderingControl::variableFromString(var);
}

std::string Client::variableToString(Variable var) const 
{
    return RenderingControl::toString(var);
}

}
}
