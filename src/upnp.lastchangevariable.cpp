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
#include <algorithm>

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

LastChangeVariable::LastChangeVariable(asio::io_service& io, ServiceType type, std::chrono::milliseconds minEventInterval)
: m_timerScheduled(false)
, m_minInterval(minEventInterval)
, m_eventMetaNamespace(serviceTypeToUrnMetadataString(type))
, m_timer(io)
{
}

void LastChangeVariable::addChangedVariable(uint32_t instanceId, const ServiceVariable& var)
{
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

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = now - m_lastUpdate;
    if (timeSinceLastUpdate > m_minInterval)
    {
        createLastChangeEvent();
    }
    else if (!m_timerScheduled)
    {
        auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(m_minInterval - timeSinceLastUpdate);
        m_timerScheduled = true;
        m_timer.expires_from_now(waitTime);
        m_timer.async_wait([this] (const boost::system::error_code& e) {
            if (e != asio::error::operation_aborted)
            {
                createLastChangeEvent();
                m_timerScheduled = false;
            }
        });
    }
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
            m_changedVariables.clear();
            m_lastUpdate = std::chrono::steady_clock::now();

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

}
