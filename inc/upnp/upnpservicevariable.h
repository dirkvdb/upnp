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

#pragma once

#include <string>
#include <cinttypes>
#include <map>
#include <sstream>

namespace upnp
{

class ServiceVariable
{
public:
    ServiceVariable() = default;
    explicit ServiceVariable(const std::string& name, const std::string& value)
    : m_name(name)
    , m_value(value)
    {
    }

    bool operator==(const ServiceVariable& other) const
    {
        // If the variable name and attibute matches we are dealing with the same
        // variable, value can change
        return  m_name == other.m_name &&
                m_attribute.first == other.m_attribute.first &&
                m_attribute.second == other.m_attribute.second;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    const std::string& getValue() const
    {
        return m_value;
    }

    const std::pair<std::string, std::string>& getAttribute() const
    {
        return m_attribute;
    }

    void addAttribute(const std::string& name, const std::string& value)
    {
        m_attribute = std::make_pair(name, value);
    }

    bool operator!() const
    {
        return !m_name.empty();
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << "<" << m_name << " val=\"" << m_value << "\"";

        if (!m_attribute.first.empty())
        {
            ss << " " << m_attribute.first << "=\"" << m_attribute.second << "\"";
        }

        ss << "/>";

        return ss.str();
    }

private:
    std::string                         m_name;
    std::string                         m_value;
    std::pair<std::string, std::string> m_attribute;
};

}
