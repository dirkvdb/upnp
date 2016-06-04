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

#include "upnp/upnp.mediarenderer.h"

#include "upnp/upnp.item.h"
#include "upnp/upnputils.h"
#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp.avtransport.typeconversions.h"

#include <sstream>

using namespace utils;
using namespace std::placeholders;
using namespace std::chrono_literals;

namespace upnp
{

namespace
{

const std::string g_NotImplemented = "NOT_IMPLEMENTED";

std::chrono::seconds parseDuration(const std::string& duration)
{
    uint32_t secs = 0u;

    auto split = stringops::tokenize(duration, ":");
    if (split.size() == 3)
    {
        secs += std::stoul(split[0]) * 3600;
        secs += std::stoul(split[1]) * 60;

        auto secondsSplit = stringops::tokenize(split[2], ".");
        secs += std::stoul(secondsSplit.front());
    }

    return std::chrono::seconds(secs);
}

Item parseCurrentTrack(const std::string& track) noexcept
{
    try
    {
        if (!track.empty())
        {
            return xml::parseItemDocument(track);
        }
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: {}", e.what());
    }

    return Item();
}

MediaRenderer::PlaybackState transportStateToPlaybackState(AVTransport::State state)
{
    switch (state)
    {
        case AVTransport::State::Playing:           return MediaRenderer::PlaybackState::Playing;
        case AVTransport::State::Recording:         return MediaRenderer::PlaybackState::Recording;
        case AVTransport::State::PausedPlayback:
        case AVTransport::State::PausedRecording:   return MediaRenderer::PlaybackState::Paused;
        case AVTransport::State::Transitioning:     return MediaRenderer::PlaybackState::Transitioning;
        case AVTransport::State::Stopped:
        case AVTransport::State::NoMediaPresent:    return MediaRenderer::PlaybackState::Stopped;

        default: throw Exception("Invalid transport state");
    }
}

MediaRenderer::Action transportActionToAction(AVTransport::Action action)
{
    switch (action)
    {
        case AVTransport::Action::Play:     return MediaRenderer::Action::Play;
        case AVTransport::Action::Stop:     return MediaRenderer::Action::Stop;
        case AVTransport::Action::Pause:    return MediaRenderer::Action::Pause;
        case AVTransport::Action::Seek:     return MediaRenderer::Action::Seek;
        case AVTransport::Action::Next:     return MediaRenderer::Action::Next;
        case AVTransport::Action::Previous: return MediaRenderer::Action::Previous;
        case AVTransport::Action::Record:   return MediaRenderer::Action::Record;

        default: throw Exception("Invalid transport action");
    }
}

MediaRenderer::PlaybackState parsePlaybackState(const std::string& state)
{
    try
    {
        return transportStateToPlaybackState(AVTransport::stateFromString(state));
    }
    catch (std::exception& e)
    {
        log::warn("Failed to parse item doc: {}", e.what());
        return MediaRenderer::PlaybackState::Stopped;
    }
}

}

MediaRenderer::MediaRenderer(IClient& client)
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

void MediaRenderer::setDevice(const std::shared_ptr<Device>& device, std::function<void(Status)> cb)
{
    assert(!m_active);
    if (m_active)
    {
        cb(Status(ErrorCode::Unexpected, "Deactivate events before setting a new renderer device"));
        return;
    }

    m_device = device;
    m_connectionMgr.setDevice(m_device, [this, cb] (Status status) {
        if (!status)
        {
            cb(status);
            log::error("Failed to set connection manager device: {}", status.what());
            return;
        }

        m_renderingControl.setDevice(m_device, [this, cb] (Status status) {
            if (!status)
            {
                cb(status);
                log::error("Failed to set rendering control device: {}", status);
                return;
            }

            if (m_device->implementsService({ ServiceType::AVTransport, 1 }))
            {
                if (!m_avTransport)
                {
                    m_avTransport = std::make_unique<AVTransport::Client>(m_client);
                }

                m_avTransport->setDevice(m_device, [this, cb] (Status status) {
                    if (!status)
                    {
                        cb(status);
                        log::error("Failed to set avtransport device: {}", status);
                    }
                    else
                    {
                        getProtocolInfo(cb);
                    }
                });
            }
            else
            {
                m_avTransport.reset();
                getProtocolInfo(cb);
            }
        });
    });
}

void MediaRenderer::getProtocolInfo(std::function<void(Status)> cb)
{
    // reset state related data
    m_connectionMgr.getProtocolInfo([this, cb] (Status status, std::vector<ProtocolInfo> info) {
        if (status)
        {
            m_protocolInfo = std::move(info);
            // make sure m3u is supported
            m_protocolInfo.push_back(ProtocolInfo("http-get:*:audio/m3u:*"));
            resetData();
        }
        else
        {
            log::error("Renderer: Failed to obtain protocol info: {}", status);
        }

        cb(status);
        log::info("Renderer: GetProtocolInfo complete {}", status);
    });
}

bool MediaRenderer::supportsPlayback(const upnp::Item& item, Resource& suggestedResource) const
{
    if (!m_device)
    {
        throw Exception("No UPnP renderer selected");
    }

    if (m_protocolInfo.empty())
    {
        // No protocol info available, let's just try
        log::warn("Renderer: No protocol info available: assuming supported");
        return true;
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
    return fmt::format("{}/{}", m_device->udn, m_device->services[ServiceType::ConnectionManager].id);
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
    m_connectionMgr.prepareForConnection(res.getProtocolInfo(),
                                         peerConnectionManager,
                                         serverConnectionId,
                                         ConnectionManager::Direction::Input,
                                         [this] (Status status, ConnectionManager::ConnectionInfo info) {
        if (status)
        {
            m_connInfo = info;
        }
    });
}

void MediaRenderer::setTransportItem(Resource& resource, std::function<void(Status)> cb)
{
    if (m_avTransport)
    {
        m_avTransport->setAVTransportURI(m_connInfo.connectionId, resource.getUrl(), "", cb);
    }
}

void MediaRenderer::setNextTransportItem(Resource& resource)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->setNextAVTransportURI(m_connInfo.connectionId, resource.getUrl(), "", nullptr);
    }
}

void MediaRenderer::play()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->play(m_connInfo.connectionId, "1", nullptr);
    }
}

void MediaRenderer::pause()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->pause(m_connInfo.connectionId, nullptr);
    }
}

void MediaRenderer::stop()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->stop(m_connInfo.connectionId, nullptr);
    }
}

void MediaRenderer::next()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->next(m_connInfo.connectionId, nullptr);
    }
}

void MediaRenderer::seekInTrack(std::chrono::seconds position)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->seek(m_connInfo.connectionId, AVTransport::SeekMode::RelativeTime, durationToString(position), nullptr);
    }
}

void MediaRenderer::previous()
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->previous(m_connInfo.connectionId, nullptr);
    }
}

void MediaRenderer::getCurrentTrackPosition(std::function<void(Status, std::chrono::seconds)> cb)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->getPositionInfo(m_connInfo.connectionId, [this, cb] (Status status, AVTransport::PositionInfo info) {
            if (status)
            {
                cb(status, parseDuration(info.relativeTime));
            }
        });
    }
}

void MediaRenderer::getPlaybackState(std::function<void(Status, PlaybackState)> cb)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->getTransportInfo(m_connInfo.connectionId, [cb] (Status status, AVTransport::TransportInfo info) {
            auto state = PlaybackState::Stopped;
            if (status)
            {
                state = transportStateToPlaybackState(info.currentTransportState);
            }

            cb(status, state);
        });
    }
}

std::string MediaRenderer::getCurrentTrackURI() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = m_avTransportInfo.find(AVTransport::Variable::CurrentTrackURI);
    return iter == m_avTransportInfo.end() ? "" : iter->second;
}

std::chrono::seconds MediaRenderer::getCurrentTrackDuration() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = m_avTransportInfo.find(AVTransport::Variable::CurrentTrackDuration);
    return iter == m_avTransportInfo.end() ? 0s : parseDuration(iter->second);
}

void MediaRenderer::getCurrentTrackInfo(std::function<void(Status, Item)> cb) const
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->getMediaInfo(m_connInfo.connectionId, [cb] (Status status, AVTransport::MediaInfo info) {
            Item track;
            if (status)
            {
                track = parseCurrentTrack(info.currentURIMetaData);
            }

            cb(status, track);
        });
    }
}

void MediaRenderer::getAvailableActions(std::function<void(Status, std::set<MediaRenderer::Action>)> cb)
{
    if (m_avTransport)
    {
        throwOnUnknownConnectionId();
        m_avTransport->getCurrentTransportActions(m_connInfo.connectionId, [cb] (Status status, const std::set<AVTransport::Action>& transpActions) {
            std::set<MediaRenderer::Action> actions;

            if (status)
            {
                for (auto& action : transpActions)
                {
                    actions.insert(transportActionToAction(action));
                }
            }

            cb(status, actions);
        });
    }
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

void MediaRenderer::setVolume(uint32_t value, std::function<void(Status)> cb)
{
    throwOnUnknownConnectionId();
    m_renderingControl.setVolume(m_connInfo.connectionId, value, cb);
}

void MediaRenderer::getVolume(std::function<void(Status status, uint32_t volume)> cb)
{
    throwOnUnknownConnectionId();
    m_renderingControl.getVolume(m_connInfo.connectionId, [cb] (Status status, uint32_t volume) {
        cb(status, volume);
    });
}

void MediaRenderer::activateEvents(std::function<void(Status)> cb)
{
    if (!m_active && m_device)
    {
        m_renderingControl.LastChangeEvent.connect(std::bind(&MediaRenderer::onRenderingControlLastChangeEvent, this, _1), this);
        m_renderingControl.subscribe([this, cb] (Status status) {
            if (m_avTransport)
            {
                m_avTransport->LastChangeEvent.connect(std::bind(&MediaRenderer::onAVTransportLastChangeEvent, this, _1), this);
                m_avTransport->subscribe([this, cb] (Status status) {
                    if (status)
                    {
                        m_active = true;
                    }

                    cb(status);
                });
            }
            else
            {
                if (status)
                {
                    m_active = true;
                }

                cb(status);
            }
        });
    }
}

void MediaRenderer::deactivateEvents(std::function<void(Status)> cb)
{
    if (m_active && m_device)
    {
        m_renderingControl.StateVariableEvent.disconnect(this);
        m_renderingControl.unsubscribe([this, cb] (Status status) {
            if (!status)
            {
                log::warn("Rendering control unsubscribe failed: {}", status.what());
            }

            if (m_avTransport)
            {
                m_avTransport->StateVariableEvent.disconnect(this);
                m_avTransport->unsubscribe([this, cb] (Status status) {
                    if (!status)
                    {
                        log::warn("AVTransport unsubscribe failed: {}", status.what());
                    }

                    m_active = false;
                    cb(status);
                });
            }
            else
            {
                m_active = false;
                cb(status);
            }
        });
    }
    else
    {
        cb(Status());
    }
}

void MediaRenderer::resetData()
{
    m_avTransportInfo.clear();
}

std::set<MediaRenderer::Action> MediaRenderer::parseAvailableActions(const std::string& actions) const
{
    std::set<Action> availableActions;

    auto actionsStrings = stringops::tokenize(actions, ',');
    for (auto& action : actionsStrings)
    {
        try
        {
            availableActions.insert(transportActionToAction(AVTransport::ServiceTraits::actionFromString(action)));
        }
        catch (std::exception& e)
        {
            log::warn(e.what());
        }
    }

    return availableActions;
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
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : vars)
        {
            m_avTransportInfo[pair.first] = pair.second;
        }
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

void MediaRenderer::throwOnUnknownConnectionId() const
{
    if (m_connInfo.connectionId == ConnectionManager::UnknownConnectionId)
    {
        throw Exception("No active renderer connection");
    }
}

}
