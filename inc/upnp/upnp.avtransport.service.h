//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include "upnp/upnp.deviceservice.h"
#include "upnp/upnp.avtransport.types.h"
#include "upnp/upnp.lastchangevariable.h"

#include <string>
#include <vector>
#include <stdexcept>

namespace upnp
{

class IAVTransport
{
public:
    virtual ~IAVTransport() = default;

    virtual void setAVTransportURI(uint32_t instanceId, const std::string& uri, const std::string& metaData) = 0;
    virtual void stop(uint32_t instanceId) = 0;
    virtual void play(uint32_t instanceId, const std::string& speed) = 0;
    virtual void seek(uint32_t instanceId, AVTransport::SeekMode mode, const std::string& target) = 0;
    virtual void next(uint32_t instanceId) = 0;
    virtual void previous(uint32_t instanceId) = 0;

    virtual void setNextAVTransportURI(uint32_t /*instanceId*/, const std::string& /*uri*/, const std::string& /*metaData*/)    { throw InvalidAction(); }
    virtual void pause(uint32_t /*instanceId*/)                                                                                 { throw InvalidAction(); }
    virtual void record(uint32_t /*instanceId*/)                                                                                { throw InvalidAction(); }
    virtual void setPlayMode(uint32_t /*instanceId*/, AVTransport::PlayMode)                                                    { throw InvalidAction(); }
    virtual void setRecordQualityMode(uint32_t /*instanceId*/, const std::string& /*mode*/)                                     { throw InvalidAction(); }
    virtual std::vector<AVTransport::Action> getCurrentTransportActions(uint32_t /*instanceId*/)                                { throw InvalidAction(); }
};

class IAVTransport3
{
public:
    virtual ~IAVTransport3() = default;

    virtual void setSyncOffset(uint32_t /*instanceId*/, const std::string& /*syncOffset*/)                                                      { throw InvalidAction(); }
    virtual void adjustSyncOffset(uint32_t /*instanceId*/, const std::string& /*adjustment*/)                                                   { throw InvalidAction(); }
    virtual void syncPlay(uint32_t /*instanceId*/, const std::string& /*speed*/, AVTransport::SeekMode /*refPositionUnit*/,
                          const std::string& /*refPosition*/, const std::string& /*refPresentatationTime*/, const std::string& /*refClockId*/)  { throw InvalidAction(); }
    virtual void syncStop(uint32_t /*instanceId*/, const std::string& /*stopTime*/, const std::string& /*refClockId*/)                          { throw InvalidAction(); }
    virtual void syncPause(uint32_t /*instanceId*/, const std::string& /*pauseTime*/, const std::string& /*refClockId*/)                        { throw InvalidAction(); }
    virtual void setStaticPlaylist(uint32_t /*instanceId*/, const std::string& /*data*/, uint32_t /*offset*/,
                                   uint32_t /*totalLength*/, const std::string& /*mimeType*/, const std::string& /*extendedType*/,
                                   const std::string& /*startObjectId*/, const std::string& /*startGroupId*/)                                   { throw InvalidAction(); }
    virtual void setStreamingPlaylist(uint32_t /*instanceId*/, const std::string& /*data*/, const std::string& /*mimeType*/,
                                      const std::string& /*extendedType*/, AVTransport::PlaylistStep /*step*/)                                  { throw InvalidAction(); }
    virtual const std::string getPlaylistInfo(uint32_t /*instanceId*/, AVTransport::PlaylistType /*type*/)                                      { throw InvalidAction(); }
};

namespace AVTransport
{

class Service : public DeviceService<Variable>
{
public:
    Service(IRootDevice& dev, IAVTransport& av);
    ~Service();

    virtual std::string getSubscriptionResponse() override;
    virtual ActionResponse onAction(const std::string& action, const std::string& request) override;

    virtual void setInstanceVariable(uint32_t id, Variable var, const std::string& value) override;

    static const char* toString(State);
    static const char* toString(PlayMode);
    static State stateFromString(const std::string& value);

protected:
    virtual const char* variableToString(Variable type) const override;
    std::string getStateVariables(uint32_t id, const std::string& variableList) const;

private:
    void throwIfNoAVTransport3Support();

    IAVTransport&                       m_avTransport;
    IAVTransport3*                      m_avTransport3;
    LastChangeVariable                  m_lastChange;
};

}
}
