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

#pragma once

#include "utils/signal.h"
#include "upnp/upnp.serviceclientbase.h"
#include "upnp/upnp.avtransport.types.h"

namespace upnp
{

class Item;
class Action;

namespace AVTransport
{

struct ServiceTraits
{
    using ActionType = AVTransport::Action;
    using VariableType = AVTransport::Variable;
    static ServiceType::Type SvcType;
    static constexpr uint8_t SvcVersion = 1;

    static Action actionFromString(const std::string& action);
    static const char* actionToString(Action action);
    static Variable variableFromString(const std::string& var);
    static const char* variableToString(Variable var);
};

class Client : public ServiceClientBase<ServiceTraits>
{
public:
    Client(IClient& client);

    void setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(upnp::Status)> cb);
    void setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData, std::function<void(upnp::Status)> cb);

    void play(int32_t connectionId, const std::string& speed, std::function<void(upnp::Status)> cb);
    void pause(int32_t connectionId, std::function<void(upnp::Status)> cb);
    void stop(int32_t connectionId, std::function<void(upnp::Status)> cb);
    void previous(int32_t connectionId, std::function<void(upnp::Status)> cb);
    void seek(int32_t connectionId, SeekMode mode, const std::string& target, std::function<void(upnp::Status)> cb);
    void next(int32_t connectionId, std::function<void(upnp::Status)> cb);

    void getPositionInfo(int32_t connectionId, std::function<void(upnp::Status, PositionInfo)> cb);
    void getMediaInfo(int32_t connectionId, std::function<void(upnp::Status, MediaInfo)> cb);
    void getTransportInfo(int32_t connectionId, std::function<void(upnp::Status, TransportInfo)> cb);
    void getCurrentTransportActions(int32_t connectionId, std::function<void(upnp::Status, std::set<Action>)> cb);

    Future<void> setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData);
    Future<void> setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData);

    Future<void> play(int32_t connectionId, const std::string& speed);
    Future<void> pause(int32_t connectionId);
    Future<void> stop(int32_t connectionId);
    Future<void> previous(int32_t connectionId);
    Future<void> seek(int32_t connectionId, SeekMode mode, const std::string& target);
    Future<void> next(int32_t connectionId);

    Future<PositionInfo> getPositionInfo(int32_t connectionId);
    Future<MediaInfo> getMediaInfo(int32_t connectionId);
    Future<TransportInfo> getTransportInfo(int32_t connectionId);
    Future<std::set<Action>> getCurrentTransportActions(int32_t connectionId);

    utils::Signal<const std::map<Variable, std::string>&> LastChangeEvent;

protected:
    void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables) override;

    std::chrono::seconds getSubscriptionTimeout() override;
};

}
}
