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

MediaRenderer::MediaRenderer(Client2& client)
: m_client(client)
, m_connectionMgr(client)
, m_renderingControl(client)
, m_active(false)
{
}

std::shared_ptr<Device> MediaRenderer::getDevice()
{
    return m_device;
}

void MediaRenderer::setDevice(const std::shared_ptr<Device>& device)
{
    try
    {
        if (m_device)
        {
            deactivateEvents();
        }

        m_device = device;
        m_connectionMgr.setDevice(device);
        m_renderingControl.setDevice(device);

        if (m_device->implementsService(ServiceType::AVTransport))
        {
            m_avTransport = std::make_unique<AVTransport::Client>(m_client);
            m_avTransport->setDevice(device);
        }

        // reset state related data
        m_protocolInfo = m_connectionMgr.getProtocolInfo();
        resetData();

        activateEvents();

        DeviceChanged(device);
    }
    catch (std::exception& e)
    {
        throw Exception("Failed to set renderer device: {}", e.what());
    }
}

bool MediaRenderer::supportsPlayback(const upnp::Item& item, Resource& suggestedResource) const
{
    if (!m_device)
    {
        throw Exception("No UPnP renderer selected");
    }

    for (auto& res : item.getResources())
    {
        auto iter = std::find_if(m_protocolInfo.begin(), m_protocolInfo.end(), [res] (const ProtocolInfo& info) {
            return info.isCompatibleWith(res.getProtocolInfo());
        });

        if (iter != m_protocolInfo.end())
        {
            suggestedResource = res;
            return true;
        }
    }

    return false;
}

std::string MediaRenderer::getPeerConnectionManager() const
{
    return fmt::format("{}/{}", m_device->m_udn, m_device->m_services[ServiceType::ConnectionManager].m_id);
}

void MediaRenderer::resetConnection()
{
    m_connInfo.connectionId = ConnectionManager::UnknownConnectionId;
}

void MediaRenderer::useDefaultConnection()
{
    m_connInfo.connectionId = ConnectionManager::DefaultConnectionId;
}

bool MediaRenderer::supportsConnectionPreparation() const
{
    return m_connectionMgr.supportsAction(ConnectionManager::Action::PrepareForConnection);
}

void MediaRenderer::prepareConnection(const Resource& res, const std::string& peerConnectionManager, uint32_t serverConnectionId)
{
    m_connInfo = m_connectionMgr.prepareForConnection(res.getProtocolInfo(), peerConnectionManager,
                                                      serverConnectionId, ConnectionManager::Direction::Input);
}

void MediaRenderer::setTransportItem(Resource& resource)
{
    if (m_avTransport)
    {
        m_avTransport->setAVTransportURI(m_connInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::setNextTransportItem(Resource& resource)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->setNextAVTransportURI(m_connInfo.connectionId, resource.getUrl());
    }
}

void MediaRenderer::play()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->play(m_connInfo.connectionId);
    }
}

void MediaRenderer::pause()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->pause(m_connInfo.connectionId);
    }
}

void MediaRenderer::stop()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->stop(m_connInfo.connectionId);
    }
}

void MediaRenderer::next()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->next(m_connInfo.connectionId);
    }
}

void MediaRenderer::seekInTrack(uint32_t position)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->seek(m_connInfo.connectionId, AVTransport::SeekMode::RelativeTime, positionToString(position));
    }
}

void MediaRenderer::previous()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->previous(m_connInfo.connectionId);
    }
}

uint32_t MediaRenderer::getCurrentTrackPosition()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        auto info = m_avTransport->getPositionInfo(m_connInfo.connectionId);
        return parseDuration(info.relativeTime);
    }

    return 0;
}

MediaRenderer::PlaybackState MediaRenderer::getPlaybackState()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        return transportStateToPlaybackState(m_avTransport->getTransportInfo(m_connInfo.connectionId).currentTransportState);
    }

    return PlaybackState::Stopped;
}

std::string MediaRenderer::getCurrentTrackURI() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_avTransportInfo.find(AVTransport::Variable::CurrentTrackURI);
    return iter == m_avTransportInfo.end() ? "" : iter->second;
}

uint32_t MediaRenderer::getCurrentTrackDuration() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = m_avTransportInfo.find(AVTransport::Variable::CurrentTrackDuration);
    return iter == m_avTransportInfo.end() ? 0 : parseDuration(iter->second);
}

Item MediaRenderer::getCurrentTrackInfo() const
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        auto info = m_avTransport->getMediaInfo(m_connInfo.connectionId);
        return parseCurrentTrack(info.currentURIMetaData);
    }

    return Item();

}

std::set<MediaRenderer::Action> MediaRenderer::getAvailableActions()
{
    std::set<MediaRenderer::Action> actions;

    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        auto transpActions = m_avTransport->getCurrentTransportActions(m_connInfo.connectionId);
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
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_avTransport ? m_avTransport->supportsAction(AVTransport::Action::SetNextAVTransportURI) : false;
}

void MediaRenderer::setVolume(uint32_t value)
{
    throwOnUnknownConnectionId();
    m_renderingControl.setVolume(m_connInfo.connectionId, value);
}

uint32_t MediaRenderer::getVolume()
{
    throwOnUnknownConnectionId();
    return m_renderingControl.getVolume(m_connInfo.connectionId);
}

void MediaRenderer::activateEvents()
{
    if (!m_active && m_device)
    {
        m_renderingControl.LastChangeEvent.connect(std::bind(&MediaRenderer::onRenderingControlLastChangeEvent, this, _1), this);
        m_renderingControl.subscribe();

        if (m_avTransport)
        {
            m_avTransport->LastChangeEvent.connect(std::bind(&MediaRenderer::onAVTransportLastChangeEvent, this, _1), this);
            m_avTransport->subscribe();
        }

        m_active = true;
    }
}

void MediaRenderer::deactivateEvents()
{
    if (m_active && m_device)
    {
        try
        {
            m_renderingControl.StateVariableEvent.disconnect(this);
            m_renderingControl.unsubscribe();
        }
        catch (std::exception& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }

        try
        {
            if (m_avTransport)
            {
                m_avTransport->StateVariableEvent.disconnect(this);
                m_avTransport->unsubscribe();
            }
        }
        catch (std::exception& e)
        {
            // this can fail if the device disappeared
            log::warn(e.what());
        }

        m_active = false;
    }
}

void MediaRenderer::resetData()
{
    m_avTransportInfo.clear();
}

std::set<MediaRenderer::Action> MediaRenderer::parseAvailableActions(const std::string& actions) const
{
    std::set<Action> availableActions;

    auto actionsStrings = stringops::tokenize(actions, ",");
    std::for_each(actionsStrings.begin(), actionsStrings.end(), [&] (const std::string& action) {
        try
        {
            availableActions.insert(transportActionToAction(m_avTransport->actionFromString(action)));
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
    });

    return availableActions;
}

Item MediaRenderer::parseCurrentTrack(const std::string& track) const
{
    try
    {
        if (!track.empty())
        {
            xml::Document doc(track);
            return xml::utils::parseItemDocument(doc);
        }
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: {}", e.what());
    }

    return Item();
}

MediaRenderer::PlaybackState MediaRenderer::parsePlaybackState(const std::string& state) const
{
    try
    {
        return transportStateToPlaybackState(AVTransport::stateFromString(state));
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: {}", e.what());
        return PlaybackState::Stopped;
    }
}

void MediaRenderer::onRenderingControlLastChangeEvent(const std::map<RenderingControl::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto iter = vars.find(RenderingControl::Variable::Volume);
    if (iter != vars.end())
    {
        VolumeChanged(utils::stringops::toNumeric<uint32_t>(iter->second));
    }
}

void MediaRenderer::onAVTransportLastChangeEvent(const std::map<AVTransport::Variable, std::string>& vars)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : vars)
    {
        m_avTransportInfo[pair.first] = pair.second;
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
    else
    {
        // No metadata present
        iter = vars.find(AVTransport::Variable::CurrentTrackURI);
        if (iter != vars.end())
        {
            // But the track uri changed, notify with empty info
            CurrentTrackChanged(Item());
        }
    }

    iter = vars.find(AVTransport::Variable::CurrentTrackDuration);
    if (iter != vars.end())
    {
        CurrentTrackDurationChanged(parseDuration(iter->second));
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

        default: throw Exception("Invalid transport action");
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

        default: throw Exception("Invalid transport state");
    }
}

void MediaRenderer::throwOnUnknownConnectionId() const
{
    if (m_connInfo.connectionId == ConnectionManager::UnknownConnectionId)
    {
        throw Exception("No active renderer connection");
    }
}

}
