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

#ifndef UPNP_AV_TRANSPORT_CLIENT_H
#define UPNP_AV_TRANSPORT_CLIENT_H

#include "utils/signal.h"
#include "upnp/upnpservicebase.h"
#include "upnp/upnpavtransporttypes.h"

namespace upnp
{

class Item;
class Action;

namespace AVTransport
{
    
class Client : public ServiceBase<Action, Variable>
{
public:
    struct PositionInfo
    {
        std::string track;
        std::string trackMetaData;
        std::string trackDuration;
        std::string trackURI;
        std::string relTime;
        std::string absTime;
        std::string relCount;
        std::string absCount;
    };
    
    Client(IClient& client);
    
    void setAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData = "");
    void setNextAVTransportURI(int32_t connectionId, const std::string& uri, const std::string& uriMetaData = "");
    
    void play(int32_t connectionId, const std::string& speed = "1");
    void pause(int32_t);
    void stop(int32_t);
    void previous(int32_t connectionId);
    void seek(int32_t connectionId, SeekMode mode, const std::string& target);
    void next(int32_t connectionId);
    PositionInfo getPositionInfo(int32_t connectionId);
    TransportInfo getTransportInfo(int32_t connectionId);
    
    utils::Signal<void(const std::map<Variable, std::string>&)> LastChangeEvent;
    
    virtual Action actionFromString(const std::string& action);
    virtual std::string actionToString(Action action);
    virtual Variable variableFromString(const std::string& var);
    virtual std::string variableToString(Variable var);
    
protected:
    void handleStateVariableEvent(Variable var, const std::map<Variable, std::string>& variables);

    ServiceType getType();
    int32_t getSubscriptionTimeout();
    
    void handleUPnPResult(int errorCode);
};

}
}

#endif
