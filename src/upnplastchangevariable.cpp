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
#include "upnp/upnp.xml.parseutils.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

using namespace utils;
using namespace rapidxml_ns;

//#define DEBUG_LAST_CHANGE_VAR

namespace upnp
{

namespace
{

const char* s_encAtr = "s:encodingStyle";
const char* s_encVal = "http://schemas.xmlsoap.org/soap/encoding/";

const char* s_xmlnsAtr = "xmlns:s";
const char* s_ns = "urn:schemas-upnp-org:event-1-0";
const char* s_propset = "e:propertyset";
const char* s_prop = "e:property";
const char* s_lastChange = "LastChange";

const char* s_event = "Event";
const char* s_instanceId = "InstanceID";
const char* s_val = "val";

xml_node<>* createServiceVariablesElement(xml_document<>& doc, uint32_t instanceId, const std::vector<ServiceVariable>& vars)
{
    auto* instance = doc.allocate_node(node_element, s_instanceId);
    instance->append_attribute(doc.allocate_attribute(s_val, doc.allocate_string(std::to_string(instanceId).c_str())));
    
    for (auto& var : vars)
    {
        auto elem = xml::serviceVariableToElement(doc, var);
        instance->append_node(elem);
    }

    return instance;
}

}

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
        xml_document<> doc;
        
        auto* propset = doc.allocate_node(node_element, s_propset);
        propset->append_attribute(doc.allocate_attribute(s_xmlnsAtr, s_ns));
        propset->append_attribute(doc.allocate_attribute(s_encAtr, s_encVal));

        auto* prop = doc.allocate_node(node_element, s_prop);
        
        
        auto* event = doc.allocate_node(node_element, s_event);
        for (auto& vars : m_changedVariables)
        {
            auto instance = createServiceVariablesElement(doc, vars.first, vars.second);
            event->append_node(instance);
        }
        
        auto eventString = xml::encode(xml::toString(*event));
        
        auto* lastChange = doc.allocate_node(node_element, s_lastChange, eventString.c_str());

        prop->append_node(lastChange);
        propset->append_node(prop);
        doc.append_node(propset);

        if (LastChangeEvent)
        {
            LastChangeEvent2(xml::toString(doc));
        }

#ifdef DEBUG_LAST_CHANGE_VAR
        utils::log::debug("LastChange event: {}", xml::toString(doc));
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
