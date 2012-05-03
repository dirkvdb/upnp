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

#include "upnp/upnpclient.h"
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
static const char* AVTransportServiceType = "urn:schemas-upnp-org:service:AVTransport:1";

    
AVTransport::AVTransport(Client& client)
: m_Client(client)
{
}

AVTransport::~AVTransport()
{
    unsubscribe();
}

void AVTransport::setDevice(std::shared_ptr<Device> device)
{
    m_Device = device;
    
    if (m_Device->implementsService(Service::AVTransport))
    {
        parseServiceDescription(m_Device->m_Services[Service::AVTransport].m_SCPDUrl);
    }
}

bool AVTransport::supportsAction(Action action) const
{
    return m_SupportedActions.find(action) != m_SupportedActions.end();
}

void AVTransport::subscribe()
{
    unsubscribe();
    
    log::debug("Subscribe to AVTransport service:", m_Device->m_FriendlyName, m_Device->m_Services[Service::AVTransport].m_EventSubscriptionURL);
    
    m_Client.UPnPEventOccurredEvent.connect(std::bind(&AVTransport::eventOccurred, this, _1), this);
    
    int ret = UpnpSubscribeAsync(m_Client, m_Device->m_Services[Service::AVTransport].m_EventSubscriptionURL.c_str(), defaultTimeout, &AVTransport::eventCb, this);
    if (ret != UPNP_E_SUCCESS)
    {
        throw std::logic_error("Failed to subscribe to UPnP device:" + numericops::toString(ret));
    }
    
    log::debug("Subscribed:", AVTransport::eventCb, &AVTransport::eventCb, this);
}

void AVTransport::unsubscribe()
{
    if (!m_Device->m_CDSubscriptionID.empty())
    {
        m_Client.UPnPEventOccurredEvent.disconnect(this);
    
        int ret = UpnpUnSubscribe(m_Client, &(m_Device->m_CDSubscriptionID[0]));
        if (ret != UPNP_E_SUCCESS)
        {
            log::warn("Failed to unsubscribe from device:", m_Device->m_FriendlyName);
        }
    }
    
    m_Variables.clear();
}

void AVTransport::setAVTransportURI(const std::string& connectionId, const std::string& uri, const std::string& uriMetaData)
{
    upnp::Action action("SetAVTransportURI", AVTransportServiceType);
	action.addArgument("InstanceID", connectionId);
    action.addArgument("CurrentURI", uri);
    action.addArgument("CurrentURIMetaData", uriMetaData);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::AVTransport].m_ControlURL.c_str(), AVTransportServiceType, nullptr, action.getActionDocument(), &result));
}

void AVTransport::play(const std::string& connectionId, const std::string& speed)
{
    upnp::Action action("Play", AVTransportServiceType);
    action.addArgument("InstanceID", connectionId);
    action.addArgument("Speed", speed);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::AVTransport].m_ControlURL.c_str(), AVTransportServiceType, nullptr, action.getActionDocument(), &result));
}

void AVTransport::stop(const std::string& connectionId)
{
    upnp::Action action("Stop", AVTransportServiceType);
    action.addArgument("InstanceID", connectionId);
    
    IXmlDocument result;
    handleUPnPResult(UpnpSendAction(m_Client, m_Device->m_Services[Service::AVTransport].m_ControlURL.c_str(), AVTransportServiceType, nullptr, action.getActionDocument(), &result));
}

void AVTransport::parseServiceDescription(const std::string& descriptionUrl)
{
    IXmlDocument doc;
    
    int ret = UpnpDownloadXmlDoc(descriptionUrl.c_str(), &doc);
    if (ret != UPNP_E_SUCCESS)
    {
        log::error("Error obtaining device description from", descriptionUrl, " error =", ret);
        return;
    }
    
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
    if (pEvent->Sid == m_Device->m_CDSubscriptionID)
    {
        log::debug("Event received:", pEvent->EventKey, ixmlDocumenttoString(pEvent->ChangedVariables));
        
        IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(pEvent->ChangedVariables, "LastChange");
        if (!nodeList)
        {
            log::error("Failed to find LastChange element in AVTransport event");
            return;
        }
        
        IXmlDocument doc = ixmlParseBuffer(getFirstElementValue(nodeList, "LastChange").c_str());
        if (!doc)
        {
            log::error("Failed to parse AVTRANSPORT event");
        }
        
        try
        {
            auto values = getEventValues(doc);
            for (auto& i : values)
            {
                try
                {
                    m_Variables[variableFromString(i.first)] = i.second;
                    log::debug(i.first, i.second);
                }
                catch (std::exception& e)
                {
                    log::warn("Unknown event variable ignored:", i.first);
                }
            }
        }
        catch (std::exception& e)
        {
            log::error("Failed to parse AVTRANSPORT event variables:", e.what());
        }
    }
}

int AVTransport::eventCb(Upnp_EventType eventType, void* pEvent, void* pInstance)
{
    AVTransport* av = reinterpret_cast<AVTransport*>(pInstance);
    
    log::info("eventcb:", eventType);
    
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
                log::info(pSubEvent->Sid);
                av->m_Device->m_CDSubscriptionID = pSubEvent->Sid;
                log::info("Successfully subscribed to", av->m_Device->m_FriendlyName, "id =", pSubEvent->Sid);
            }
            else
            {
                av->m_Device->m_CDSubscriptionID.clear();
                log::error("Subscription id for device is empty");
            }
        }
        
        break;
    }
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
    if (var == "LastChange")                     return Variable::LastChange;
    
    throw std::logic_error("Unknown AVTransport variable:" + var);
}
    
}
