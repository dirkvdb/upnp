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

#include "upnp/upnp.avtransport.client.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnputils.h"

#include "utils/log.h"
#include "utils/numericoperations.h"

#include "rapidxml.hpp"

namespace upnp
{
namespace AVTransport
{

using namespace utils;
using namespace rapidxml_ns;
using namespace std::placeholders;

static const std::chrono::seconds g_subscriptionTimeout(1801);

namespace
{

template <typename Func, typename... Args>
void invoke(Func&& func, Args&&... args)
{
    if (func)
    {
        func(std::forward<Args&&>(args)...);
    }
}

std::function<void(int32_t, std::string)> stripResponse(std::function<void(int32_t)> cb)
{
    return [cb] (int32_t status, const std::string&) {
        invoke(cb, status);
    };
}

std::string optionalChildValue(xml_node<>& node, const char* child)
{
    std::string result;

    auto* childNode = node.first_node(child);
    if (childNode && childNode->value())
    {
        result = std::string(childNode->value(), childNode->value_size());
    }

    return child;
}

}

Client::Client(IClient2& client)
: ServiceClientBase(client)
{
}

void Client::setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(int32_t)> cb)
{
    executeAction(Action::SetAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                               {"CurrentURI", uri},
                                               {"CurrentURIMetaData", uriMetaData} }, stripResponse(cb));
}

void Client::setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(int32_t)> cb)
{
    executeAction(Action::SetNextAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                                   {"NextURI", uri},
                                                   {"NextURIMetaData", uriMetaData} }, stripResponse(cb));
}

void Client::play(int32_t connectionId, const std::string& speed, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Play, { {"InstanceID", std::to_string(connectionId)},
                                               {"Speed", speed} }, stripResponse(cb));
}

void Client::pause(int32_t connectionId, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Pause, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::stop(int32_t connectionId, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Stop, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::next(int32_t connectionId, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Next, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::previous(int32_t connectionId, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Previous, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::seek(int32_t connectionId, SeekMode mode, const std::string& target, std::function<void(int32_t)> cb)
{
    executeAction(AVTransport::Action::Seek, { {"InstanceID", std::to_string(connectionId)},
                                               {"Unit", toString(mode)},
                                               {"Target", target} }, stripResponse(cb));
}

void Client::getPositionInfo(int32_t connectionId, std::function<void(int32_t, PositionInfo)> cb)
{
    executeAction(Action::GetPositionInfo, { {"InstanceID", std::to_string(connectionId)} }, [cb] (int32_t status, const std::string& response) {
        PositionInfo info;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");

                info.track = xml::optionalStringToUnsignedNumeric<uint32_t>(optionalChildValue(body, "Track"));
                info.trackDuration = optionalChildValue(body, "TrackDuration");
                info.trackMetaData = optionalChildValue(body, "TrackMetaData");
                info.trackURI = optionalChildValue(body, "TrackURI");
                info.relativeTime = optionalChildValue(body, "RelTime");
                info.absoluteTime = optionalChildValue(body, "AbsTime");
                info.relativeCount = xml::optionalStringToUnsignedNumeric<int32_t>(optionalChildValue(body, "RelCount"));
                info.absoluteCount = xml::optionalStringToUnsignedNumeric<int32_t>(optionalChildValue(body, "AbsCount"));
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getMediaInfo(int32_t connectionId, std::function<void(int32_t, MediaInfo)> cb)
{
    executeAction(Action::GetCurrentTransportActions, { {"InstanceID", std::to_string(connectionId)} }, [cb] (int32_t status, const std::string& response) {
        MediaInfo info;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");

                info.numberOfTracks = xml::utils::optionalStringToUnsignedNumeric<uint32_t>(optionalChildValue(body, "NrTracks"));
                info.mediaDuration = optionalChildValue(body, "MediaDuration");
                info.currentURI = optionalChildValue(body, "CurrentUri");
                info.currentURIMetaData = optionalChildValue(body, "CurrentUriMetaData");
                info.nextURI = optionalChildValue(body, "NextURI");
                info.nextURIMetaData = optionalChildValue(body, "NextURIMetaData");
                info.playMedium = optionalChildValue(body, "PlayMedium");
                info.recordMedium = optionalChildValue(body, "RecordMedium");
                info.writeStatus = optionalChildValue(body, "WriteStatus");
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getTransportInfo(int32_t connectionId, std::function<void(int32_t, TransportInfo)> cb)
{
    executeAction(Action::GetTransportInfo, { {"InstanceID", std::to_string(connectionId)} }, [cb] (int32_t status, const std::string& response) {
        TransportInfo info;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& body = doc.first_node_ref("Envelope").first_node_ref("Body");

                auto* child = body.first_node("CurrentTransportState");
                if (child)
                {
                    info.currentTransportState = stateFromString(std::string(child->value(), child->value_size()));
                }

                child = body.first_node("CurrentTransportStatus");
                if (child)
                {
                    info.currentTransportState = stateFromString(std::string(child->value(), child->value_size()));
                }

                child = body.first_node("CurrentSpeed");
                if (child)
                {
                    info.currentSpeed = std::string(child->value(), child->value_size());
                }
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getCurrentTransportActions(int32_t connectionId, std::function<void(int32_t, std::set<Action>)> cb)
{
    executeAction(Action::GetCurrentTransportActions, { {"InstanceID", std::to_string(connectionId)} }, [this, cb] (int32_t status, std::string response) {
        std::set<Action> actions;
        if (status == 200)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(&response.front());
                auto& actionsNode = doc.first_node_ref("Envelope").first_node_ref("Body").first_node_ref("Actions");

                for(auto& action : stringops::tokenize(std::string(actionsNode.value(), actionsNode.value_size()), ','))
                {
                    try
                    {
                        actions.insert(actionFromString(action));
                    }
                    catch (std::exception& e)
                    {
                        log::warn(e.what());
                    }
                }
            }
            catch(std::exception& e)
            {
                log::error(e.what());
                status = -1;
            }
        }

        invoke(cb, status, actions);
    });
}

ServiceType Client::getType()
{
    return ServiceType::AVTransport;
}

std::chrono::seconds Client::getSubscriptionTimeout()
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
