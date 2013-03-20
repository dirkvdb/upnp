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
    
    virtual void SetAVTransportURI(const std::string& uri) = 0;
    virtual void GetMediaInfo() = 0;
    virtual void GetTransportInfo() = 0;
    virtual void GetPositionInfo() = 0;
    virtual void GetDeviceCapabilities() = 0;
    virtual void GetTransportSettings() = 0;
    virtual void Stop() = 0;
    virtual void Play() = 0;
    virtual void Seek() = 0;
    virtual void Next() = 0;
    virtual void Previous() = 0;
    
    virtual void SetNextAVTransportURI(const std::string& uri)                  { throw InvalidActionException(); }
    virtual void Pause()                                                        { throw InvalidActionException(); }
    virtual void Record()                                                       { throw InvalidActionException(); }
    virtual void SetPlayMode()                                                  { throw InvalidActionException(); }
    virtual void SetRecordQualityMode()                                         { throw InvalidActionException(); }
    virtual std::vector<AVTransport::Action> GetCurrentTransportActions()       { throw InvalidActionException(); }
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
