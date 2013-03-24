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

#ifndef UPNP_SERVICE_VARIABLE_H
#define UPNP_SERVICE_VARIABLE_H

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
    : m_Name(name)
    , m_Value(value)
    {
    }
    
    bool operator==(const ServiceVariable& other) const
    {
        // If the variable name and attibute matches we are dealing with the same
        // variable, value can change
        return  m_Name == other.m_Name &&
                m_Attribute.first == other.m_Attribute.first &&
                m_Attribute.second == other.m_Attribute.second;
    }
    
    std::string getValue() const
    {
        return m_Value;
    }
    
    void addAttribute(const std::string& name, const std::string& value)
    {
        m_Attribute = std::make_pair(name, value);
    }
    
    std::string toString() const
    {
        std::stringstream ss;
        ss << "<" << m_Name << " val=\"" << m_Value << "\"";
        
        if (!m_Attribute.first.empty())
        {
            ss << " " << m_Attribute.first << "=\"" << m_Attribute.second << "\"";
        }
        
        ss << "/>";
        
        return ss.str();
    }

private:
    std::string                         m_Name;
    std::string                         m_Value;
    std::pair<std::string, std::string> m_Attribute;
};

}

#endif
