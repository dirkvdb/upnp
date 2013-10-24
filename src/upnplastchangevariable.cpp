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

#include "upnp/upnplastchangevariable.h"

#include <chrono>

#include "utils/log.h"
#include "upnp/upnpxmlutils.h"

using namespace utils;

//#define DEBUG_LAST_CHANGE_VAR

namespace upnp
{

LastChangeVariable::LastChangeVariable(ServiceType type, std::chrono::milliseconds minEventIntervalInMilliSecs)
: m_Thread(std::bind(&LastChangeVariable::variableThread, this))
, m_MinInterval(minEventIntervalInMilliSecs)
, m_Stop(false)
, m_EventMetaNamespace(serviceTypeToUrnMetadataString(type))
{
}

LastChangeVariable::~LastChangeVariable()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Stop = true;
        m_Condition.notify_all();
    }
    
    if (m_Thread.joinable())
    {
        m_Thread.join();
    }
}

void LastChangeVariable::addChangedVariable(uint32_t instanceId, const ServiceVariable& var)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (var.getName().empty())
    {
        log::error("Empty var added %s", var.toString());
        return;
    }
    
    auto& vars = m_ChangedVariables[instanceId];
    auto iter = std::find(vars.begin(), vars.end(), var);
    if (iter == vars.end())
    {
        vars.push_back(var);
    }
    else
    {
        *iter = var;
    }
    
    m_Condition.notify_all();
}

void LastChangeVariable::createLastChangeEvent()
{
    try
    {
        const std::string ns = "urn:schemas-upnp-org:event-1-0";

        xml::Document doc;
        auto propertySet    = doc.createElement("e:propertyset");
        auto property       = doc.createElement("e:property");
        auto lastChange     = doc.createElement("LastChange");
        
        propertySet.addAttribute("xmlns:e", ns);
        
        auto event = doc.createElement("Event");
        event.addAttribute("xmlns", m_EventMetaNamespace);
        
        for (auto& vars : m_ChangedVariables)
        {
            auto instance = xml::utils::createServiceVariablesElement(doc, vars.first, vars.second);
            event.appendChild(instance);
        }
        
        auto lastChangeValue = doc.createNode(event.toString());

        lastChange.appendChild(lastChangeValue);
        property.appendChild(lastChange);
        propertySet.appendChild(property);
        doc.appendChild(propertySet);

        LastChangeEvent(doc);
        
#ifdef DEBUG_LAST_CHANGE_VAR
        utils::log::debug("LastChange event: %s", event.toString());
#endif
    }
    catch (std::exception& e)
    {
        log::error("Failed to create LastChange notification: %s", e.what());
    }
}

void LastChangeVariable::variableThread()
{
    while (!m_Stop)
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Condition.wait(lock, [this] { return (!m_ChangedVariables.empty()) || m_Stop; });
        if (m_Stop)
        {
            continue;
        }
        
        createLastChangeEvent();
        m_ChangedVariables.clear();
        
        // Wait at least MinInterval before a new update is sent or until a stop is requested
        m_Condition.wait_for(lock, m_MinInterval, [this] { return m_Stop; });
    }
}

}
