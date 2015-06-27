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
: m_thread(std::bind(&LastChangeVariable::variableThread, this))
, m_minInterval(minEventIntervalInMilliSecs)
, m_stop(false)
, m_eventMetaNamespace(serviceTypeToUrnMetadataString(type))
{
}

LastChangeVariable::~LastChangeVariable()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
        m_condition.notify_all();
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void LastChangeVariable::addChangedVariable(uint32_t instanceId, const ServiceVariable& var)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (var.getName().empty())
    {
        log::error("Empty var added {}", var.toString());
        return;
    }

    auto& vars = m_changedVariables[instanceId];
    auto iter = std::find(vars.begin(), vars.end(), var);
    if (iter == vars.end())
    {
        vars.push_back(var);
    }
    else
    {
        *iter = var;
    }

    m_condition.notify_all();
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
        event.addAttribute("xmlns", m_eventMetaNamespace);

        for (auto& vars : m_changedVariables)
        {
            auto instance = xml::utils::createServiceVariablesElement(doc, vars.first, vars.second);
            event.appendChild(instance);
        }

        auto lastChangeValue = doc.createNode(event.toString());

        lastChange.appendChild(lastChangeValue);
        property.appendChild(lastChange);
        propertySet.appendChild(property);
        doc.appendChild(propertySet);

        if (LastChangeEvent)
        {
            LastChangeEvent(doc);
        }

#ifdef DEBUG_LAST_CHANGE_VAR
        utils::log::debug("LastChange event: {}", event.toString());
#endif
    }
    catch (std::exception& e)
    {
        log::error("Failed to create LastChange notification: {}", e.what());
    }
}

void LastChangeVariable::variableThread()
{
    while (!m_stop)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return (!m_changedVariables.empty()) || m_stop; });
        if (m_stop)
        {
            continue;
        }

        createLastChangeEvent();
        m_changedVariables.clear();

        // Wait at least MinInterval before a new update is sent or until a stop is requested
        m_condition.wait_for(lock, m_minInterval, [this] { return m_stop; });
    }
}

}
