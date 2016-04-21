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
#include "utils/log.h"

#include <pugixml.hpp>

namespace upnp
{

struct Action2::Pimpl
{
    std::string name;
    std::string url;
    ServiceType serviceType;
    pugi::xml_document doc;
    pugi::xml_node action;
};

Action2::Action2(const std::string& name, const std::string& url, ServiceType serviceType)
: m_pimpl(std::make_unique<Pimpl>())
{
    m_pimpl->name = name;
    m_pimpl->url = url;
    m_pimpl->serviceType = serviceType;

    auto env = m_pimpl->doc.append_child("s:Envelope");
    env.append_attribute("xmlns:s").set_value("http://schemas.xmlsoap.org/soap/envelope/");
    env.append_attribute("s:encodingStyle").set_value("http://schemas.xmlsoap.org/soap/encoding/");
    auto body = env.append_child("s:Body");
    m_pimpl->action = body.append_child(("u:" + name).c_str());
    m_pimpl->action.append_attribute("xmlns:u").set_value(getServiceTypeUrn().c_str());
}

Action2::~Action2() = default;

void Action2::addArgument(const std::string& name, const std::string& value)
{
    if (!m_pimpl->action.append_child(name.c_str()).text().set(value.c_str()))
    {
        throw std::runtime_error(fmt::format("Failed to add action to UPnP request: {}", name));
    }
}

std::string Action2::toString() const
{
    class StringWriter : public pugi::xml_writer
    {
    public:
        StringWriter(std::string& str) : m_str(str) {}
        void write(const void* data, size_t size) override
        {
            m_str.append(static_cast<const char*>(data), size);
        }
    private:
        std::string& m_str;
    };

    std::string result;
    StringWriter writer(result);
    m_pimpl->doc.save(writer, "", pugi::format_raw);
    return result;
}

std::string Action2::getName() const
{
    return m_pimpl->name;
}

std::string Action2::getUrl() const
{
    return m_pimpl->url;
}

std::string Action2::getServiceTypeUrn() const
{
    return serviceTypeToUrnTypeString(m_pimpl->serviceType);
}

ServiceType Action2::getServiceType() const
{
    return m_pimpl->serviceType;
}

bool Action2::operator==(const Action2& other) const
{
    if (m_pimpl->doc.empty() && !other.m_pimpl->doc.empty())
    {
        return false;
    }

    return toString() == other.toString();
}

}