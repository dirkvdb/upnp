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

#ifndef UPNP_AVTRANSPORT_SERVICE_H
#define UPNP_AVTRANSPORT_SERVICE_H

#include "upnp/upnpdeviceservice.h"
#include "upnp/upnpavtransporttypes.h"

#include <string>
#include <vector>
#include <stdexcept>

namespace upnp
{

class IAVTransport
{
public:
    virtual ~IAVTransport() {}
    
    virtual void setAVTransportURI(uint32_t instanceId, const std::string& uri, const std::string& metaData) = 0;
    virtual AVTransport::MediaInfo getMediaInfo(uint32_t instanceId) = 0;
    virtual AVTransport::TransportInfo getTransportInfo(uint32_t instanceId) = 0;
    virtual AVTransport::PositionInfo getPositionInfo(uint32_t instanceId) = 0;
    virtual AVTransport::DeviceCapabilities getDeviceCapabilities(uint32_t instanceId) = 0;
    virtual AVTransport::TransportSettings getTransportSettings(uint32_t instanceId) = 0;
    virtual void stop(uint32_t instanceId) = 0;
    virtual void play(uint32_t instanceId, const std::string& speed) = 0;
    virtual void seek(uint32_t instanceId, AVTransport::SeekMode mode, const std::string& target) = 0;
    virtual void next(uint32_t instanceId) = 0;
    virtual void previous(uint32_t instanceId) = 0;
    
    virtual void setNextAVTransportURI(uint32_t instanceId, const std::string& uri, const std::string& metaData)    { throw InvalidActionException(); }
    virtual void pause(uint32_t instanceId)                                                                         { throw InvalidActionException(); }
    virtual void record(uint32_t instanceId)                                                                        { throw InvalidActionException(); }
    virtual void setPlayMode(uint32_t instanceId, AVTransport::PlayMode)                                            { throw InvalidActionException(); }
    virtual void setRecordQualityMode(uint32_t instanceId, const std::string& mode)                                 { throw InvalidActionException(); }
    virtual std::vector<AVTransport::Action> getCurrentTransportActions(uint32_t)                                   { throw InvalidActionException(); }
};

namespace AVTransport
{

class Service : public DeviceService
{
public:
    Service(IAVTransport& av);
    
    virtual ActionResponse onAction(const std::string& action, const xml::Document& request);
    
private:
    IAVTransport&  m_avTransport;
};

}
}

#endif
