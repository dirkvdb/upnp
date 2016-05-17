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

#include "upnp/upnp.renderingcontrol.client.h"

#include "upnp/upnputils.h"
#include "upnp/upnp.controlpoint.h"
#include "upnp.renderingcontrol.typeconversions.h"

#include "utils/numericoperations.h"
#include "rapidxml.hpp"

namespace upnp
{
namespace RenderingControl
{

using namespace utils;
using namespace rapidxml_ns;
using namespace std::placeholders;

static const std::chrono::seconds s_subscriptionTimeout(1801);

ServiceType::Type ServiceTraits::SvcType = ServiceType::RenderingControl;

Action ServiceTraits::actionFromString(const std::string& action)
{
    return RenderingControl::actionFromString(action);
}

const char* ServiceTraits::actionToString(Action action)
{
    return RenderingControl::toString(action);
}

Variable ServiceTraits::variableFromString(const std::string& var)
{
    return RenderingControl::variableFromString(var);
}

const char* ServiceTraits::variableToString(Variable var)
{
    return RenderingControl::toString(var);
}

Client::Client(upnp::IClient2& client)
: ServiceClientBase(client)
, m_minVolume(0)
, m_maxVolume(100)
{
}

void Client::setVolume(int32_t connectionId, uint32_t value, std::function<void(Status status)> cb)
{
    numericops::clip(value, m_minVolume, m_maxVolume);
    executeAction(Action::SetVolume, { {"InstanceID", std::to_string(connectionId)},
                                       {"Channel", "Master"},
                                       {"DesiredVolume", numericops::toString(value)} }, [cb] (Status status, std::string) {
        if (cb)
        {
            cb(status);
        }
   });
}

void Client::getVolume(int32_t connectionId, std::function<void(Status status, uint32_t volume)> cb)
{
    executeAction(Action::GetVolume, { {"InstanceID", std::to_string(connectionId)},
                                       {"Channel", "Master"} }, [cb] (Status status, const std::string& response) {
        uint32_t volume = 0;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(response.c_str());
                auto& volumeNode = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("CurrentVolume");
                volume = stringops::toNumeric<uint32_t>(volumeNode.value_string());
            }
            catch (std::exception& e)
            {
                status = Status(ErrorCode::Unexpected, "Failed to parse volume");
            }
        }

        cb(status, volume);
    });
}

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return s_subscriptionTimeout;
}

void Client::processServiceDescription(const std::string& descriptionUrl, std::function<void(Status)> cb)
{
    ServiceClientBase::processServiceDescription(descriptionUrl, [this, cb] (Status status) {
        if (status)
        {
            for (auto& variable : m_stateVariables)
            {
                if (variable.name == toString(Variable::Volume))
                {
                    if (variable.valueRange)
                    {
                        m_minVolume = variable.valueRange->minimumValue;
                        m_maxVolume = variable.valueRange->maximumValue;
                    }
                }
            }
        }

        cb(status);
    });
}

void Client::handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables)
{
    if (var == Variable::LastChange)
    {
        LastChangeEvent(variables);
    }
}

// void Client::handleUPnPResult(int errorCode)
// {
//     if (errorCode == UPNP_E_SUCCESS) return;

//     switch (errorCode)
//     {
//         case 702: throw Exception(errorCode, "Invalid instance id");
//         default: upnp::handleUPnPResult(errorCode);
//     }
// }

}
}
