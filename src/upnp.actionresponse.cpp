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

#include "upnp/upnp.actionresponse.h"
#include "utils/log.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include "upnp/upnp.xml.parseutils.h"

namespace upnp
{

using namespace rapidxml_ns;

namespace
{

const char* g_envelopeTag = "s:Envelope";
const char* g_bodyTag = "s:Body";

const char* g_xmlnsAtr = "xmlns:s";
const char* g_xmlnsVal = "http://schemas.xmlsoap.org/soap/envelope/";
const char* g_encAtr = "s:encodingStyle";
const char* g_encVal = "http://schemas.xmlsoap.org/soap/encoding/";

const char* g_xmlnsuAtr = "xmlns:u";

}

struct ActionResponse::Pimpl
{
    std::string name;
    std::string url;
    ServiceType serviceType;
    xml_document<> doc;
    xml_node<>* action = nullptr;
};

ActionResponse::ActionResponse(const std::string& name, ServiceType serviceType)
: m_pimpl(std::make_unique<Pimpl>())
{
    m_pimpl->name = "u:" + name + "Response";
    m_pimpl->serviceType = serviceType;

    auto* env = m_pimpl->doc.allocate_node(node_element, g_envelopeTag);
    env->append_attribute(m_pimpl->doc.allocate_attribute(g_xmlnsAtr, g_xmlnsVal));
    env->append_attribute(m_pimpl->doc.allocate_attribute(g_encAtr, g_encVal));

    auto* body = m_pimpl->doc.allocate_node(node_element, g_bodyTag);
    m_pimpl->action = m_pimpl->doc.allocate_node(node_element, m_pimpl->name.c_str());
    m_pimpl->action->append_attribute(m_pimpl->doc.allocate_attribute(g_xmlnsuAtr, getServiceTypeUrn()));

    body->append_node(m_pimpl->action);
    env->append_node(body);
    m_pimpl->doc.append_node(env);
}

ActionResponse::ActionResponse(ActionResponse&& other) = default;

ActionResponse::~ActionResponse() = default;

void ActionResponse::addArgument(const std::string& name, const std::string& value)
{
    auto* arg = m_pimpl->doc.allocate_node(node_element, m_pimpl->doc.allocate_string(name.c_str()), m_pimpl->doc.allocate_string(value.c_str()));
    m_pimpl->action->append_node(arg);
}

std::string ActionResponse::toString() const
{
    return xml::toString(m_pimpl->doc);
}

std::string ActionResponse::getName() const
{
    return m_pimpl->name.substr(2, m_pimpl->name.size() - 10); // strip the stored namespace and "response"
}

const char* ActionResponse::getServiceTypeUrn() const
{
    return serviceTypeToUrnTypeString(m_pimpl->serviceType);
}

ServiceType ActionResponse::getServiceType() const
{
    return m_pimpl->serviceType;
}

bool ActionResponse::operator==(const ActionResponse& other) const
{
    return toString() == other.toString();
}

}
