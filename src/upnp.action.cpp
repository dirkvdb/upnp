//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later versions
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "upnp/upnp.action.h"

#include <sstream>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

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

struct Action2::Pimpl
{
    std::string name;
    std::string url;
    ServiceType serviceType;
    xml_document<> doc;
    xml_node<>* action = nullptr;
};

Action2::Action2(const std::string& name, const std::string& url, ServiceType serviceType)
: m_pimpl(std::make_unique<Pimpl>())
{
    m_pimpl->name = "u:" + name;
    m_pimpl->url = url;
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

Action2::~Action2() = default;

void Action2::addArgument(const std::string& name, const std::string& value)
{
    auto* arg = m_pimpl->doc.allocate_node(node_element, m_pimpl->doc.allocate_string(name.c_str()), m_pimpl->doc.allocate_string(value.c_str()));
    m_pimpl->action->append_node(arg);
}

std::string Action2::toString() const
{

    std::string result("<?xml version=\"1.0\"?>");
    rapidxml_ns::print(std::back_inserter(result), m_pimpl->doc, print_no_indenting);
    return result;
}

std::string Action2::getName() const
{
    return m_pimpl->name.substr(2); // strip the stored namespace
}

std::string Action2::getUrl() const
{
    return m_pimpl->url;
}

const char* Action2::getServiceTypeUrn() const
{
    return serviceTypeToUrnTypeString(m_pimpl->serviceType);
}

ServiceType Action2::getServiceType() const
{
    return m_pimpl->serviceType;
}

bool Action2::operator==(const Action2& other) const
{
    return toString() == other.toString();
}

}