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

#include "upnp/upnpavtransport.h"

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

static const int32_t defaultTimeout = 1801;
    
AVTransport::AVTransport(IClient& client)
: m_Client(client)
{
}

AVTransport::~AVTransport()
{
    try
    {
        unsubscribe();
    }
    catch (std::exception& e)
    {
        log::error(e.what());
    }
}

void AVTransport::setDevice(const std::shared_ptr<Device>& device)
{
    if (device->implementsService(ServiceType::AVTransport))
    {
        m_Service = device->m_Services[ServiceType::AVTransport];
        parseServiceDescription(m_Service.m_SCPDUrl);
    }
}

bool AVTransport::supportsAction(Action action) const
{
    return m_SupportedActions.find(action) != m_SupportedActions.end();
}

void AVTransport::subscribe()
{
    unsubscribe();
    
    m_Client.UPnPEventOccurredEvent.connect(std::bind(&AVTransport::eventOccurred, this, _1), this);
    m_Client.subscribeToService(m_Service.m_EventSubscriptionURL, defaultTimeout, &AVTransport::eventCb, this);
}

void AVTransport::unsubscribe()
{
    if (!m_Service.m_EventSubscriptionID.empty())
    {
        m_Client.UPnPEventOccurredEvent.disconnect(this);
        m_Client.unsubscribeFromService(&(m_Service.m_EventSubscriptionID[0]));
        m_Service.m_EventSubscriptionID.clear();
    }
}

void AVTransport::setAVTransportURI(const std::string& connectionId, const std::string& uri, const std::string& uriMetaData)
{
    std::map<std::string, std::string> args;
    args.insert(std::make_pair("CurrentURI", uri));
    args.insert(std::make_pair("CurrentURIMetaData", uriMetaData));
    executeAction(Action::SetAVTransportURI, connectionId, args);
}

void AVTransport::play(const std::string& connectionId, const std::string& speed)
{
    executeAction(Action::Play, connectionId, { {"Speed", speed} });
}

void AVTransport::pause(const std::string& connectionId)
{
    executeAction(Action::Pause, connectionId);
}

void AVTransport::stop(const std::string& connectionId)
{
    executeAction(Action::Stop, connectionId);
}

void AVTransport::next(const std::string& connectionId)
{
    executeAction(Action::Next, connectionId);
}

void AVTransport::previous(const std::string& connectionId)
{
    executeAction(Action::Previous, connectionId);
}

AVTransport::TransportInfo AVTransport::getTransportInfo(const std::string& connectionId)
{
    xml::Document doc = executeAction(Action::GetTransportInfo, connectionId);
    
    TransportInfo info;
    info.currentTransportState      = doc.getChildElementValue("CurrentTransportState");
    info.currentTransportStatus     = doc.getChildElementValue("CurrentTransportStatus");
    info.currentSpeed               = doc.getChildElementValue("CurrentSpeed");
    
    return info;
}

AVTransport::PositionInfo AVTransport::getPositionInfo(const std::string& connectionId)
{
    xml::Document doc = executeAction(Action::GetPositionInfo, connectionId);
    
    PositionInfo info;
    info.track          = doc.getChildElementValue("Track");
    info.trackDuration  = doc.getChildElementValue("TrackDuration");
    info.trackMetaData  = doc.getChildElementValue("TrackMetaData");
    info.trackURI       = doc.getChildElementValue("TrackURI");
    info.relTime        = doc.getChildElementValue("RelTime");
    info.absTime        = doc.getChildElementValue("AbsTime");
    info.relCount       = doc.getChildElementValue("RelCount");
    info.absCount       = doc.getChildElementValue("AbsCount");
    
    return info;
}

xml::Document AVTransport::executeAction(Action actionType, const std::string& connectionId, const std::map<std::string, std::string>& args)
{
    upnp::Action action(actionToString(actionType), m_Service.m_ControlURL, ServiceType::AVTransport);
    action.addArgument("InstanceID", connectionId);
    
    for (auto& arg : args)
    {
        action.addArgument(arg.first, arg.second);
    }
    
    try
    {
        return m_Client.sendAction(action);
    }
    catch (int32_t errorCode)
    {
        handleUPnPResult(errorCode);
    }
    
    assert(false);
    return xml::Document();
}

void AVTransport::parseServiceDescription(const std::string& descriptionUrl)
{
    xml::Document doc = m_Client.downloadXmlDocument(descriptionUrl);
    
    for (auto& action : getActionsFromDescription(doc))
    {
        try
        {
            m_SupportedActions.insert(actionFromString(action));
        }
        catch (std::exception& e)
        {
            log::error(e.what());
        }
    }
}

void AVTransport::eventOccurred(Upnp_Event* pEvent)
{
    if (pEvent->Sid == m_Service.m_EventSubscriptionID)
    {
        try
        {
            xml::Document doc(pEvent->ChangedVariables, xml::Document::NoOwnership);
            xml::Document changeDoc(doc.getChildElementValueRecursive("LastChange"));
            
            std::map<Variable, std::string> vars;
            auto values = getEventValues(changeDoc);
            for (auto& i : values)
            {
                try
                {
                    vars[variableFromString(i.first)] = i.second;
#ifdef DEBUG_AVTRANSPORT
                    log::debug(i.first, i.second);
#endif
                }
                catch (std::exception& e)
                {
                    log::warn("Unknown AVTransport event variable ignored:", i.first);
                }
            }
            
            LastChangedEvent(vars);
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse AVTransport event variables:", e.what());
        }
    }
}

int AVTransport::eventCb(Upnp_EventType eventType, void* pEvent, void* pInstance)
{
    AVTransport* av = reinterpret_cast<AVTransport*>(pInstance);
    
    switch (eventType)
    {
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    {
        Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
        if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
        {
            log::error("Error in Event Subscribe Callback:", pSubEvent->ErrCode);
        }
        else
        {
            if (pSubEvent->Sid)
            {
                av->m_Service.m_EventSubscriptionID = pSubEvent->Sid;
            }
            else
            {
                av->m_Service.m_EventSubscriptionID.clear();
                log::error("Subscription id for device is empty");
            }
        }
        break;
    }
    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
    {
        Upnp_Event_Subscribe* pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
        
        try
        {
            Upnp_SID newSID;
            int32_t timeout = defaultTimeout;
            
            av->m_Client.subscribeToService(pSubEvent->PublisherUrl, timeout, newSID);
            av->m_Service.m_EventSubscriptionID = newSID;
            
            log::info("AVTransport subscription renewed: \n", newSID);
        }
        catch (std::exception& e)
        {
            log::error(std::string("Failed to renew AVTransport event subscription: ") + e.what());
        }
        
        break;
    }
    case UPNP_EVENT_RENEWAL_COMPLETE:
        log::info("AVTransport subscription renewal complete");
        break;
    default:
        log::info("Unhandled action:", eventType);
        break;
    }
    
    return 0;
}

AVTransport::Action AVTransport::actionFromString(const std::string& action)
{
    if (action == "SetAVTransportURI")              return Action::SetAVTransportURI;
    if (action == "SetNextAVTransportURI")          return Action::SetNextAVTransportURI;
    if (action == "GetMediaInfo")                   return Action::GetMediaInfo;
    if (action == "GetTransportInfo")               return Action::GetTransportInfo;
    if (action == "GetPositionInfo")                return Action::GetPositionInfo;
    if (action == "GetDeviceCapabilities")          return Action::GetDeviceCapabilities;
    if (action == "GetTransportSettings")           return Action::GetTransportSettings;
    if (action == "Stop")                           return Action::Stop;
    if (action == "Play")                           return Action::Play;
    if (action == "Pause")                          return Action::Pause;
    if (action == "Record")                         return Action::Record;
    if (action == "Seek")                           return Action::Seek;
    if (action == "Next")                           return Action::Next;
    if (action == "Previous")                       return Action::Previous;
    if (action == "SetPlayMode")                    return Action::SetPlayMode;
    if (action == "SetRecordQualityMode")           return Action::SetRecordQualityMode;
    if (action == "GetCurrentTransportActions")     return Action::GetCurrentTransportActions;

    throw std::logic_error("Unknown AVTransport action:" + action);
}

std::string AVTransport::actionToString(Action action)
{
    switch (action)
    {
        case Action::SetAVTransportURI:             return "SetAVTransportURI";
        case Action::SetNextAVTransportURI:         return "SetNextAVTransportURI";
        case Action::GetMediaInfo:                  return "GetMediaInfo";
        case Action::GetTransportInfo:              return "GetTransportInfo";
        case Action::GetPositionInfo:               return "GetPositionInfo";
        case Action::GetDeviceCapabilities:         return "GetDeviceCapabilities";
        case Action::GetTransportSettings:          return "GetTransportSettings";
        case Action::Stop:                          return "Stop";
        case Action::Play:                          return "Play";
        case Action::Pause:                         return "Pause";
        case Action::Record:                        return "Record";
        case Action::Seek:                          return "Seek";
        case Action::Next:                          return "Next";
        case Action::Previous:                      return "Previous";  
        case Action::SetPlayMode:                   return "SetPlayMode";
        case Action::SetRecordQualityMode:          return "SetRecordQualityMode";
        case Action::GetCurrentTransportActions:    return "GetCurrentTransportActions";
        default:
            throw std::logic_error("Invalid AVTransport action");
    }
}

AVTransport::Variable AVTransport::variableFromString(const std::string& var)
{
    if (var == "TransportState")                 return Variable::TransportState;
    if (var == "TransportStatus")                return Variable::TransportStatus;
    if (var == "PlaybackStorageMedium")          return Variable::PlaybackStorageMedium;
    if (var == "PossiblePlaybackStorageMedia")   return Variable::PossiblePlaybackStorageMedia;
    if (var == "PossibleRecordStorageMedia")     return Variable::PossibleRecordStorageMedia;
    if (var == "CurrentPlayMode")                return Variable::CurrentPlayMode;
    if (var == "TransportPlaySpeed")             return Variable::TransportPlaySpeed;
    if (var == "RecordStorageMedium")            return Variable::RecordStorageMedium;
    if (var == "RecordMediumWriteStatus")        return Variable::RecordMediumWriteStatus;
    if (var == "PossibleRecordQualityModes")     return Variable::PossibleRecordQualityModes;
    if (var == "CurrentRecordQualityMode")       return Variable::CurrentRecordQualityMode;
    if (var == "NumberOfTracks")                 return Variable::NumberOfTracks;
    if (var == "CurrentTrack")                   return Variable::CurrentTrack;
    if (var == "CurrentTrackDuration")           return Variable::CurrentTrackDuration;
    if (var == "CurrentMediaDuration")           return Variable::CurrentMediaDuration;
    if (var == "CurrentTrackURI")                return Variable::CurrentTrackURI;
    if (var == "CurrentTrackMetaData")           return Variable::CurrentTrackMetaData;
    if (var == "AVTransportURI")                 return Variable::AVTransportURI;
    if (var == "AVTransportURIMetaData")         return Variable::AVTransportURIMetaData;
    if (var == "NextAVTransportURI")             return Variable::NextAVTransportURI;
    if (var == "NextAVTransportURIMetaData")     return Variable::NextAVTransportURIMetaData;
    if (var == "CurrentTransportActions")        return Variable::CurrentTransportActions;
    if (var == "RelativeTimePosition")           return Variable::RelativeTimePosition;
    if (var == "AbsoluteTimePosition")           return Variable::AbsoluteTimePosition;
    if (var == "RelativeCounterPosition")        return Variable::RelativeCounterPosition;
    if (var == "AbsoluteCounterPosition")        return Variable::AbsoluteCounterPosition;
    if (var == "A_ARG_TYPE_SeekMode")            return Variable::ArgumentTypeSeekMode;
    if (var == "A_ARG_TYPE_SeekTarget")          return Variable::ArgumentTypeSeekTarget;
    if (var == "A_ARG_TYPE_InstanceID")          return Variable::ArgumentTypeInstanceId;
    if (var == "LastChange")                     return Variable::LastChange;
    
    throw std::logic_error("Unknown AVTransport variable:" + var);
}

void AVTransport::handleUPnPResult(int errorCode)
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
