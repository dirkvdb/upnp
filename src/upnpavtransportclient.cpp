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

#include "upnp/upnpavtransportclient.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnputils.h"
#include "upnp/upnpaction.h"
#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/numericoperations.h"

using namespace utils;
using namespace std::placeholders;

namespace upnp
{
namespace AVTransport
{

static const int32_t g_subscriptionTimeout = 1801;
    
Client::Client(IClient& client)
: ServiceBase(client)
{
}

void Client::setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData)
{
    executeAction(Action::SetAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                               {"CurrentURI", uri},
                                               {"CurrentURIMetaData", uriMetaData} });
}

void Client::setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData)
{
    executeAction(Action::SetNextAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                                   {"NextURI", uri},
                                                   {"NextURIMetaData", uriMetaData} });
}

void Client::play(int32_t connectionId, const std::string& speed)
{
    executeAction(AVTransport::Action::Play, { {"InstanceID", std::to_string(connectionId)},
                                               {"Speed", speed} });
}

void Client::pause(int32_t connectionId)
{
    executeAction(AVTransport::Action::Pause, { {"InstanceID", std::to_string(connectionId)} });
}

void Client::stop(int32_t connectionId)
{
    executeAction(AVTransport::Action::Stop, { {"InstanceID", std::to_string(connectionId)} });
}

void Client::next(int32_t connectionId)
{
    executeAction(AVTransport::Action::Next, { {"InstanceID", std::to_string(connectionId)} });
}

void Client::previous(int32_t connectionId)
{
    executeAction(AVTransport::Action::Previous, { {"InstanceID", std::to_string(connectionId)} });
}

TransportInfo Client::getTransportInfo(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetTransportInfo, { {"InstanceID", std::to_string(connectionId)} });
    xml::Element response = doc.getFirstChild();
    
    TransportInfo info;
    for (xml::Element elem : response.getChildNodes())
    {
             if (elem.getName() == "CurrentTransportState")   info.currentTransportState    = stateFromString(elem.getValue());
        else if (elem.getName() == "CurrentTransportStatus")  info.currentTransportStatus   = statusFromString(elem.getValue());
        else if (elem.getName() == "CurrentSpeed")            info.currentSpeed             = elem.getValue();
    }
    
    return info;
}

Client::PositionInfo Client::getPositionInfo(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetPositionInfo, { {"InstanceID", std::to_string(connectionId)} });
    xml::Element response = doc.getFirstChild();
    
    PositionInfo info;
    for (xml::Element elem : response.getChildNodes())
    {
             if (elem.getName() == "Track")          info.track          = elem.getValue();
        else if (elem.getName() == "TrackDuration")  info.trackDuration  = elem.getValue();
        else if (elem.getName() == "TrackMetaData")  info.trackMetaData  = elem.getValue();
        else if (elem.getName() == "TrackURI")       info.trackURI       = elem.getValue();
        else if (elem.getName() == "RelTime")        info.relTime        = elem.getValue();
        else if (elem.getName() == "AbsTime")        info.absTime        = elem.getValue();
        else if (elem.getName() == "RelCount")       info.relCount       = elem.getValue();
        else if (elem.getName() == "AbsCount")       info.absCount       = elem.getValue();
    }
    
    return info;
}

ServiceType Client::getType()
{
    return ServiceType::AVTransport;
}

int32_t Client::getSubscriptionTimeout()
{
    return g_subscriptionTimeout;
}

void Client::handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables)
{
    if (var == Variable::LastChange)
    {
        LastChangeEvent(variables);
    }
}

Action Client::actionFromString(const std::string& action)
{
    return AVTransport::actionFromString(action);
}

std::string Client::actionToString(Action action)
{
    return AVTransport::toString(action);
}

Variable Client::variableFromString(const std::string& var)
{
    return AVTransport::variableFromString(var);
}

std::string Client::variableToString(Variable var)
{
    return AVTransport::toString(var);
}
    
void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;
    
    switch (errorCode)
    {
        case 701: throw std::logic_error("Playback transition not supported at this moment");
        case 702: throw std::logic_error("No content found in media item");
        case 703: throw std::logic_error("The media could not be read");
        case 704: throw std::logic_error("Storage format not supported by the device");
        case 705: throw std::logic_error("The device is locked");
        case 706: throw std::logic_error("Error when writing media");
        case 707: throw std::logic_error("Media is not writable");
        case 708: throw std::logic_error("Format is not supported for recording");
        case 709: throw std::logic_error("The media is full");
        case 710: throw std::logic_error("Seek mode is not supported");
        case 711: throw std::logic_error("Illegal seek target");
        case 712: throw std::logic_error("Play mode is not supported");
        case 713: throw std::logic_error("Record quality is not supported");
        case 714: throw std::logic_error("Unsupported MIME-type");
        case 715: throw std::logic_error("Resource is already being played");
        case 716: throw std::logic_error("Resource not found");
        case 717: throw std::logic_error("Play speed not supported");
        case 718: throw std::logic_error("Invalid instance id");
        
        default: upnp::handleUPnPResult(errorCode);
    }
}
    
}
}
