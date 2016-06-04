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

#include "upnp/upnp.lastchangevariable.h"

#include <chrono>

#include "utils/log.h"
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace utils;
using namespace std::string_literals;

//#define DEBUG_LAST_CHANGE_VAR

namespace upnp
{

using namespace utils;
using namespace rapidxml_ns;

static const char* s_xmlnsAtr = "xmlns:e";
static const char* s_instanceIdNode = "InstanceID";
static const char* s_event = "Event";
static const char* s_valAtr         = "val";

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
        if (LastChangeEvent)
        {
            std::vector<std::pair<std::string, std::string>> vars;

            xml_document<> doc;
            auto* event = doc.allocate_node(node_element, s_event);
            event->append_attribute(doc.allocate_attribute(s_xmlnsAtr, m_eventMetaNamespace.c_str()));

            for (auto& vars : m_changedVariables)
            {
                auto* instance = doc.allocate_node(node_element, s_instanceIdNode);
                auto* indexString = doc.allocate_string(std::to_string(vars.first).c_str());
                instance->append_attribute(doc.allocate_attribute(s_valAtr, indexString));

                for (auto& var : vars.second)
                {
                    instance->append_node(xml::serviceVariableToElement(doc, var));
                }
            }

            vars.emplace_back("LastChange"s, xml::encode(xml::toString(doc)));

            LastChangeEvent(xml::createNotificationXml(vars));

#ifdef DEBUG_LAST_CHANGE_VAR
            utils::log::debug("LastChange event: {}", xmlDoc);
#endif
        }
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
