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
: ServiceClientBase(client)
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

void Client::seek(int32_t connectionId, SeekMode mode, const std::string& target)
{
    executeAction(AVTransport::Action::Seek, { {"InstanceID", std::to_string(connectionId)},
                                               {"Unit", toString(mode)},
                                               {"Target", target} });
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

PositionInfo Client::getPositionInfo(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetPositionInfo, { {"InstanceID", std::to_string(connectionId)} });
    xml::Element response = doc.getFirstChild();

    PositionInfo info;
    for (xml::Element elem : response.getChildNodes())
    {
             if (elem.getName() == "Track")          info.track          = xml::utils::optionalStringToUnsignedNumeric<uint32_t>(elem.getValue());
        else if (elem.getName() == "TrackDuration")  info.trackDuration  = elem.getValue();
        else if (elem.getName() == "TrackMetaData")  info.trackMetaData  = elem.getValue();
        else if (elem.getName() == "TrackURI")       info.trackURI       = elem.getValue();
        else if (elem.getName() == "RelTime")        info.relativeTime   = elem.getValue();
        else if (elem.getName() == "AbsTime")        info.absoluteTime   = elem.getValue();
        else if (elem.getName() == "RelCount")       info.relativeCount  = xml::utils::optionalStringToNumeric<int32_t>(elem.getValue());
        else if (elem.getName() == "AbsCount")       info.absoluteCount  = xml::utils::optionalStringToNumeric<int32_t>(elem.getValue());
    }

    return info;
}

MediaInfo Client::getMediaInfo(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetCurrentTransportActions, { {"InstanceID", std::to_string(connectionId)} });
    xml::Element response = doc.getFirstChild();

    MediaInfo info;
    for (xml::Element elem : response.getChildNodes())
    {
             if (elem.getName() == "NrTracks")              info.numberOfTracks     = xml::utils::optionalStringToUnsignedNumeric<uint32_t>(elem.getValue());
        else if (elem.getName() == "MediaDuration")         info.mediaDuration      = elem.getValue();
        else if (elem.getName() == "CurrentUri")            info.currentURI         = elem.getValue();
        else if (elem.getName() == "CurrentUriMetaData")    info.currentURIMetaData = elem.getValue();
        else if (elem.getName() == "NextURI")               info.nextURI            = elem.getValue();
        else if (elem.getName() == "NextURIMetaData")       info.nextURIMetaData    = elem.getValue();
        else if (elem.getName() == "PlayMedium")            info.playMedium         = elem.getValue();
        else if (elem.getName() == "RecordMedium")          info.recordMedium       = elem.getValue();
        else if (elem.getName() == "WriteStatus")           info.writeStatus        = elem.getValue();
    }

    return info;
}

std::set<Action> Client::getCurrentTransportActions(int32_t connectionId)
{
    xml::Document doc = executeAction(Action::GetCurrentTransportActions, { {"InstanceID", std::to_string(connectionId)} });
    xml::Element response = doc.getFirstChild();

    std::set<Action> actions;
    for (xml::Element elem : response.getChildNodes())
    {
        if (elem.getName() == "Actions")
        {
            auto actionStrings = stringops::tokenize(elem.getValue(), ",");
            std::for_each(actionStrings.begin(), actionStrings.end(), [&] (const std::string& action) {
                try
                {
                    actions.insert(actionFromString(action));
                }
                catch (std::exception& e)
                {
                    log::warn(e.what());
                }
            });

        }
    }

    return actions;
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

Action Client::actionFromString(const std::string& action) const
{
    return AVTransport::actionFromString(action);
}

std::string Client::actionToString(Action action) const
{
    return AVTransport::toString(action);
}

Variable Client::variableFromString(const std::string& var) const
{
    return AVTransport::variableFromString(var);
}

std::string Client::variableToString(Variable var) const
{
    return AVTransport::toString(var);
}

void Client::handleUPnPResult(int errorCode)
{
    if (errorCode == UPNP_E_SUCCESS) return;

    switch (errorCode)
    {
        case 701: throw Exception(errorCode, "Playback transition not supported at this moment");
        case 702: throw Exception(errorCode, "No content found in media item");
        case 703: throw Exception(errorCode, "The media could not be read");
        case 704: throw Exception(errorCode, "Storage format not supported by the device");
        case 705: throw Exception(errorCode, "The device is locked");
        case 706: throw Exception(errorCode, "Error when writing media");
        case 707: throw Exception(errorCode, "Media is not writable");
        case 708: throw Exception(errorCode, "Format is not supported for recording");
        case 709: throw Exception(errorCode, "The media is full");
        case 710: throw Exception(errorCode, "Seek mode is not supported");
        case 711: throw Exception(errorCode, "Illegal seek target");
        case 712: throw Exception(errorCode, "Play mode is not supported");
        case 713: throw Exception(errorCode, "Record quality is not supported");
        case 714: throw Exception(errorCode, "Unsupported MIME-type");
        case 715: throw Exception(errorCode, "Resource is already being played");
        case 716: throw Exception(errorCode, "Resource not found");
        case 717: throw Exception(errorCode, "Play speed not supported");
        case 718: throw Exception(errorCode, "Invalid instance id");

        default: upnp::handleUPnPResult(errorCode);
    }
}

}
}
