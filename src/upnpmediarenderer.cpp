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

#include "upnp/upnpmediarenderer.h"

#include "upnp/upnpitem.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include <sstream>

using namespace utils;
using namespace std::placeholders;


namespace upnp
{

static const std::string g_NotImplemented = "NOT_IMPLEMENTED";

MediaRenderer::MediaRenderer(IClient& client)
: m_Client(client)
, m_ConnectionMgr(client)
, m_RenderingControl(client)
, m_Active(false)
{
}

std::shared_ptr<Device> MediaRenderer::getDevice()
{
    return m_Device;
}

void MediaRenderer::setDevice(const std::shared_ptr<Device>& device)
{
    try
    {
        if (m_Device)
        {
            deactivateEvents();
        }

        m_Device = device;
        m_ConnectionMgr.setDevice(device);
        m_RenderingControl.setDevice(device);
        
        if (m_Device->implementsService(ServiceType::AVTransport))
        {
            m_AVtransport = std::make_unique<AVTransport::Client>(m_Client);
            m_AVtransport->setDevice(device);
        }
        
        // reset state related data
        m_ProtocolInfo = m_ConnectionMgr.getProtocolInfo();
        resetData();
        
        activateEvents();
        
        DeviceChanged(device);
    }
    catch (std::exception& e)
    {
        throw std::logic_error(stringops::format("Failed to set renderer device: %s", e.what()));
    }
}

bool MediaRenderer::supportsPlayback(const std::shared_ptr<const upnp::Item>& item, Resource& suggestedResource) const
{
    if (!m_Device)
    {
        throw std::logic_error("No UPnP renderer selected");
    }

    for (auto& res : item->getResources())
    {
        auto iter = std::find_if(m_ProtocolInfo.begin(), m_ProtocolInfo.end(), [res] (const ProtocolInfo& info) {
            return info.isCompatibleWith(res.getProtocolInfo());
        });
        
        if (iter != m_ProtocolInfo.end())
        {
            suggestedResource = res;
            return true;
        }
    }
    
    return false;
}

std::string MediaRenderer::getPeerConnectionManager() const
{
    std::stringstream ss;
    ss << m_Device->m_UDN << "/" << m_Device->m_Services[ServiceType::ConnectionManager].m_Id;
    
    return ss.str();
}

void MediaRenderer::resetConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void MediaRenderer::useDefaultConnection()
{
    m_ConnInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

bool MediaRenderer::supportsConnectionPreparation() const
{
    return m_ConnectionMgr.supportsAction(ConnectionManager::Action::PrepareForConnection);
}

void MediaRenderer::prepareConnection(const Resource& res, const std::string& peerConnectionManager, uint32_t serverConnectionId)
{
    m_ConnInfo = m_ConnectionMgr.prepareForConnection(res.getProtocolInfo(), peerConnectionManager,
                                                      serverConnectionId, ConnectionManager::Direction::Input);
}

void MediaRenderer::setTransportItem(Resource& resource)
{
    if (m_AVtransport)
    {
        m_AVtransport->setAVTransportURI(m_ConnInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::setNextTransportItem(Resource& resource)
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->setNextAVTransportURI(m_ConnInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::play()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->play(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::pause()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->pause(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::stop()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->stop(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::next()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->next(m_ConnInfo.connectionId);
    }
}

void MediaRenderer::seekInTrack(uint32_t position)
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->seek(m_ConnInfo.connectionId, AVTransport::SeekMode::RelativeTime, positionToString(position));
    }
}

void MediaRenderer::previous()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        m_AVtransport->previous(m_ConnInfo.connectionId);
    }
}

uint32_t MediaRenderer::getCurrentTrackPosition()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        auto info = m_AVtransport->getPositionInfo(m_ConnInfo.connectionId);
        return parseDuration(info.relativeTime);
    }
    
    return 0;
}

MediaRenderer::PlaybackState MediaRenderer::getPlaybackState()
{
    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        return transportStateToPlaybackState(m_AVtransport->getTransportInfo(m_ConnInfo.connectionId).currentTransportState);
    }
    
    return PlaybackState::Stopped;
}

std::string MediaRenderer::getCurrentTrackURI() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTrackURI);
    return iter == m_AvTransportInfo.end() ? "" : iter->second;
}

uint32_t MediaRenderer::getCurrentTrackDuration() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = m_AvTransportInfo.find(AVTransport::Variable::CurrentTrackDuration);
    return iter == m_AvTransportInfo.end() ? 0 : parseDuration(iter->second);
}

ItemPtr MediaRenderer::getCurrentTrackInfo() const
{
    ItemPtr item;

    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        auto info = m_AVtransport->getMediaInfo(m_ConnInfo.connectionId);
        item = parseCurrentTrack(info.currentURIMetaData);
    }
    
    return item;
    
}

std::set<MediaRenderer::Action> MediaRenderer::getAvailableActions()
{
    std::set<MediaRenderer::Action> actions;

    if (m_AVtransport)
    {
        throwOnUnknownConnectionId();
        auto transpActions = m_AVtransport->getCurrentTransportActions(m_ConnInfo.connectionId);
        for (auto& action : transpActions)
        {
            actions.insert(transportActionToAction(action));
        }
    }
    
    return actions;
}

bool MediaRenderer::isActionAvailable(const std::set<Action>& actions, Action action)
{
    return actions.find(action) != actions.end();
}

bool MediaRenderer::supportsQueueItem() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_AVtransport ? m_AVtransport->supportsAction(AVTransport::Action::SetNextAVTransportURI) : false;
}

void MediaRenderer::setVolume(uint32_t value)
{
    throwOnUnknownConnectionId();
    m_RenderingControl.setVolume(m_ConnInfo.connectionId, value);
}

uint32_t MediaRenderer::getVolume()
{
    throwOnUnknownConnectionId();
    return m_RenderingControl.getVolume(m_ConnInfo.connectionId);
}

void MediaRenderer::activateEvents()
{
    if (!m_Active && m_Device)
    {
        m_RenderingControl.LastChangeEvent.connect(std::bind(&MediaRenderer::onRenderingControlLastChangeEvent, this, _1), this);
        m_RenderingControl.subscribe();

        if (m_AVtransport)
        {
            m_AVtransport->LastChangeEvent.connect(std::bind(&MediaRenderer::onAVTransportLastChangeEvent, this, _1), this);
            m_AVtransport->subscribe();
        }
        
        m_Active = true;
    }
}

void MediaRenderer::deactivateEvents()
{
    if (m_Active && m_Device)
    {
        try
        {
            m_RenderingControl.StateVariableEvent.disconnect(this);
            m_RenderingControl.unsubscribe();
        }
        catch (std::logic_error& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }
    
        try
        {
            if (m_AVtransport)
            {
                m_AVtransport->StateVariableEvent.disconnect(this);
                m_AVtransport->unsubscribe();
            }
        }
        catch (std::logic_error& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }
        
        m_Active = false;
    }
}

void MediaRenderer::resetData()
{
    m_AvTransportInfo.clear();
}

std::set<MediaRenderer::Action> MediaRenderer::parseAvailableActions(const std::string& actions) const
{
    std::set<Action> availableActions;
    
    auto actionsStrings = stringops::tokenize(actions, ",");
    std::for_each(actionsStrings.begin(), actionsStrings.end(), [&] (const std::string& action) {
        try
        {
            availableActions.insert(transportActionToAction(m_AVtransport->actionFromString(action)));
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
    });
    
    return availableActions;
}

ItemPtr MediaRenderer::parseCurrentTrack(const std::string& track) const
{
    try
    {
        xml::Document doc(track);
        return xml::utils::parseItemDocument(doc);
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: %s", e.what());
        return ItemPtr();
    }
}

MediaRenderer::PlaybackState MediaRenderer::parsePlaybackState(const std::string& state) const
{
    try
    {
        return transportStateToPlaybackState(AVTransport::stateFromString(state));
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: %s", e.what());
        return PlaybackState::Stopped;
    }
}

void MediaRenderer::onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto iter = vars.find(RenderingControl::Variable::Volume);
    if (iter != vars.end())
    {
        VolumeChanged(utils::stringops::toNumeric<uint32_t>(iter->second));
    }
}

void MediaRenderer::onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto& pair : vars)
    {
        m_AvTransportInfo[pair.first] = pair.second;
    }

    auto iter = vars.find(AVTransport::Variable::CurrentTransportActions);
    if (iter != vars.end())
    {
        AvailableActionsChanged(parseAvailableActions(iter->second));
    }
    
    iter = vars.find(AVTransport::Variable::CurrentTrackMetaData);
    if (iter != vars.end())
    {
        CurrentTrackChanged(parseCurrentTrack(iter->second));
    }
    
    
    iter = vars.find(AVTransport::Variable::TransportState);
    if (iter != vars.end())
    {
        PlaybackStateChanged(parsePlaybackState(iter->second));
    }
}

uint32_t MediaRenderer::parseDuration(const std::string& duration) const
{
    uint32_t seconds = 0;

    auto split = stringops::tokenize(duration, ":");
    if (split.size() == 3)
    {
        seconds += std::stoul(split[0]) * 3600;
        seconds += std::stoul(split[1]) * 60;
        
        auto secondsSplit = stringops::tokenize(split[2], ".");
        seconds += std::stoul(secondsSplit.front());
    }
    
    return seconds;
}

std::string MediaRenderer::positionToString(uint32_t position) const
{
    uint32_t hours   = position / 3600;
    position -= hours * 3600;

    uint32_t minutes = position / 60;
    uint32_t seconds = position % 60;


    std::stringstream ss;

    if (hours > 0)
    {
        ss << hours << ':';
    }
    
    ss << std::setw(2) << std::setfill('0') << minutes << ':' << std::setw(2) << std::setfill('0')  << seconds;
    return ss.str();
}

MediaRenderer::Action MediaRenderer::transportActionToAction(AVTransport::Action action)
{
    switch (action)
    {
        case AVTransport::Action::Play:     return Action::Play;
        case AVTransport::Action::Stop:     return Action::Stop;
        case AVTransport::Action::Pause:    return Action::Pause;
        case AVTransport::Action::Seek:     return Action::Seek;
        case AVTransport::Action::Next:     return Action::Next;
        case AVTransport::Action::Previous: return Action::Previous;
        case AVTransport::Action::Record:   return Action::Record;

        default: throw std::logic_error("Invalid transport action");
    }
}

MediaRenderer::PlaybackState MediaRenderer::transportStateToPlaybackState(AVTransport::State state)
{
    switch (state)
    {
        case AVTransport::State::Playing:           return PlaybackState::Playing;
        case AVTransport::State::Recording:         return PlaybackState::Recording;
        case AVTransport::State::PausedPlayback:
        case AVTransport::State::PausedRecording:   return PlaybackState::Paused;
        case AVTransport::State::Transitioning:     return PlaybackState::Transitioning;
        case AVTransport::State::Stopped:
        case AVTransport::State::NoMediaPresent:    return PlaybackState::Stopped;
        
        default: throw std::logic_error("Invalid transport state");
    }
}

void MediaRenderer::throwOnUnknownConnectionId() const
{
    if (m_ConnInfo.connectionId == ConnectionManager::UnknownConnectionId)
    {
        throw std::logic_error("No active renderer connection");
    }
}

}
