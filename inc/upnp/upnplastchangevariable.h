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

#ifndef UPNP_LAST_CHANGE_VARIABLE_H
#define UPNP_LAST_CHANGE_VARIABLE_H

#include <string>
#include <cinttypes>
#include <map>
#include <chrono>
#include <thread>
#include <memory>
#include <iostream>
#include <condition_variable>

#include "utils/signal.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpxmlutils.h"
#include "upnp/upnpservicevariable.h"

namespace upnp
{

class LastChangeVariable
{
public:
    LastChangeVariable(ServiceType type, std::chrono::milliseconds minEventInterval);
    ~LastChangeVariable();
    
    void addChangedVariable(uint32_t instanceId, const ServiceVariable& var);
    
    std::function<void(const xml::Document&)> LastChangeEvent;
    
private:
    void variableThread();
    void createLastChangeEvent();

    std::mutex                                          m_Mutex;
    std::condition_variable                             m_Condition;
    std::thread                                         m_Thread;
    std::map<uint32_t, std::vector<ServiceVariable>>    m_ChangedVariables;
    std::chrono::milliseconds                           m_MinInterval;
    bool                                                m_Stop;
    std::string                                         m_EventMetaNamespace;
};

}


#endif
