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
#include "upnp/upnp.utils.h"
#include "upnp.avtransport.typeconversions.h"

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

static const std::chrono::seconds s_subscriptionTimeout(1801);

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

std::function<void(upnp::Status, std::string)> stripResponse(std::function<void(upnp::Status)> cb)
{
    return [cb] (upnp::Status status, const std::string&) {
        invoke(cb, status);
    };
}

}

ServiceType::Type ServiceTraits::SvcType = ServiceType::AVTransport;

Action ServiceTraits::actionFromString(const std::string& action)
{
    return AVTransport::actionFromString(action);
}

const char* ServiceTraits::actionToString(Action action)
{
    return AVTransport::actionToString(action);
}

Variable ServiceTraits::variableFromString(const std::string& var)
{
    return AVTransport::variableFromString(var);
}

const char* ServiceTraits::variableToString(Variable var)
{
    return AVTransport::variableToString(var);
}

Client::Client(IClient& client)
: ServiceClientBase(client)
{
}

void Client::setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(upnp::Status)> cb)
{
    executeAction(Action::SetAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                               {"CurrentURI", uri},
                                               {"CurrentURIMetaData", uriMetaData} }, stripResponse(cb));
}

void Client::setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(upnp::Status)> cb)
{
    executeAction(Action::SetNextAVTransportURI, { {"InstanceID", std::to_string(connectionId)},
                                                   {"NextURI", uri},
                                                   {"NextURIMetaData", uriMetaData} }, stripResponse(cb));
}

void Client::play(int32_t connectionId, const std::string& speed, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Play, { {"InstanceID", std::to_string(connectionId)},
                                               {"Speed", speed} }, stripResponse(cb));
}

void Client::pause(int32_t connectionId, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Pause, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::stop(int32_t connectionId, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Stop, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::next(int32_t connectionId, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Next, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::previous(int32_t connectionId, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Previous, { {"InstanceID", std::to_string(connectionId)} }, stripResponse(cb));
}

void Client::seek(int32_t connectionId, SeekMode mode, const std::string& target, std::function<void(upnp::Status)> cb)
{
    executeAction(AVTransport::Action::Seek, { {"InstanceID", std::to_string(connectionId)},
                                               {"Unit", toString(mode)},
                                               {"Target", target} }, stripResponse(cb));
}

void Client::getPositionInfo(int32_t connectionId, std::function<void(upnp::Status, PositionInfo)> cb)
{
    executeAction(Action::GetPositionInfo, { {"InstanceID", std::to_string(connectionId)} }, [cb] (upnp::Status status, const std::string& response) {
        PositionInfo info;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& responseNode = doc.first_node_ref().first_node_ref().first_node_ref();
                info.track = xml::optionalStringToUnsignedNumeric<uint32_t>(xml::optionalChildValue(responseNode, "Track"));
                info.trackDuration = xml::optionalChildValue(responseNode, "TrackDuration");
                info.trackMetaData = xml::optionalChildValue(responseNode, "TrackMetaData");
                info.trackURI = xml::optionalChildValue(responseNode, "TrackURI");
                info.relativeTime = xml::optionalChildValue(responseNode, "RelTime");
                info.absoluteTime = xml::optionalChildValue(responseNode, "AbsTime");
                info.relativeCount = xml::optionalStringToUnsignedNumeric<int32_t>(xml::optionalChildValue(responseNode, "RelCount"));
                info.absoluteCount = xml::optionalStringToUnsignedNumeric<int32_t>(xml::optionalChildValue(responseNode, "AbsCount"));
            }
            catch(std::exception& e)
            {
                status = upnp::Status(ErrorCode::Unexpected, e.what());
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getMediaInfo(int32_t connectionId, std::function<void(upnp::Status, MediaInfo)> cb)
{
    executeAction(Action::GetMediaInfo, { {"InstanceID", std::to_string(connectionId)} }, [cb] (upnp::Status status, const std::string& response) {
        MediaInfo info;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& responseNode = doc.first_node_ref().first_node_ref().first_node_ref();

                info.numberOfTracks = xml::optionalStringToUnsignedNumeric<uint32_t>(xml::optionalChildValue(responseNode, "NrTracks"));
                info.mediaDuration = xml::optionalChildValue(responseNode, "MediaDuration");
                info.currentURI = xml::optionalChildValue(responseNode, "CurrentUri");
                info.currentURIMetaData = xml::optionalChildValue(responseNode, "CurrentUriMetaData");
                info.nextURI = xml::optionalChildValue(responseNode, "NextURI");
                info.nextURIMetaData = xml::optionalChildValue(responseNode, "NextURIMetaData");
                info.playMedium = xml::optionalChildValue(responseNode, "PlayMedium");
                info.recordMedium = xml::optionalChildValue(responseNode, "RecordMedium");
                info.writeStatus = xml::optionalChildValue(responseNode, "WriteStatus");
            }
            catch(std::exception& e)
            {
                status = upnp::Status(ErrorCode::Unexpected, e.what());
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getTransportInfo(int32_t connectionId, std::function<void(upnp::Status, TransportInfo)> cb)
{
    executeAction(Action::GetTransportInfo, { {"InstanceID", std::to_string(connectionId)} }, [cb] (upnp::Status status, const std::string& response) {
        TransportInfo info;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(response.c_str()));
                auto& responseNode = doc.first_node_ref().first_node_ref().first_node_ref();
                auto* child = responseNode.first_node("CurrentTransportState");
                if (child)
                {
                    info.currentTransportState = stateFromString(child->value_view());
                }

                child = responseNode.first_node("CurrentTransportStatus");
                if (child)
                {
                    info.currentTransportStatus = statusFromString(child->value_view());
                }

                child = responseNode.first_node("CurrentSpeed");
                if (child)
                {
                    info.currentSpeed = std::string(child->value(), child->value_size());
                }
            }
            catch(std::exception& e)
            {
                status = upnp::Status(ErrorCode::Unexpected, e.what());
            }
        }

        invoke(cb, status, info);
    });
}

void Client::getCurrentTransportActions(int32_t connectionId, std::function<void(upnp::Status, std::set<Action>)> cb)
{
    executeAction(Action::GetCurrentTransportActions, { {"InstanceID", std::to_string(connectionId)} }, [this, cb] (upnp::Status status, std::string response) {
        std::set<Action> actions;
        if (status)
        {
            try
            {
                xml_document<> doc;
                doc.parse<parse_non_destructive>(&response.front());
                auto& actionsNode = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("Actions");
                for (auto& action : stringops::tokenize(std::string(actionsNode.value(), actionsNode.value_size()), ','))
                {
                    try
                    {
                        actions.insert(AVTransport::actionFromString(action));
                    }
                    catch (std::exception& e)
                    {
                        log::warn(e.what());
                    }
                }
            }
            catch(std::exception& e)
            {
                status = upnp::Status(ErrorCode::Unexpected, e.what());
            }
        }

        invoke(cb, status, actions);
    });
}

std::chrono::seconds Client::getSubscriptionTimeout()
{
    return s_subscriptionTimeout;
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
//         case 701: throw Exception(errorCode, "Playback transition not supported at this moment");
//         case 702: throw Exception(errorCode, "No content found in media item");
//         case 703: throw Exception(errorCode, "The media could not be read");
//         case 704: throw Exception(errorCode, "Storage format not supported by the device");
//         case 705: throw Exception(errorCode, "The device is locked");
//         case 706: throw Exception(errorCode, "Error when writing media");
//         case 707: throw Exception(errorCode, "Media is not writable");
//         case 708: throw Exception(errorCode, "Format is not supported for recording");
//         case 709: throw Exception(errorCode, "The media is full");
//         case 710: throw Exception(errorCode, "Seek mode is not supported");
//         case 711: throw Exception(errorCode, "Illegal seek target");
//         case 712: throw Exception(errorCode, "Play mode is not supported");
//         case 713: throw Exception(errorCode, "Record quality is not supported");
//         case 714: throw Exception(errorCode, "Unsupported MIME-type");
//         case 715: throw Exception(errorCode, "Resource is already being played");
//         case 716: throw Exception(errorCode, "Resource not found");
//         case 717: throw Exception(errorCode, "Play speed not supported");
//         case 718: throw Exception(errorCode, "Invalid instance id");

//         default: upnp::handleUPnPResult(errorCode);
//     }
// }

}
}
