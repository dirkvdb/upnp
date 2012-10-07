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

#include "utils/log.h"
#include "upnp/upnpaction.h"

#include <stdexcept>

namespace upnp
{
    
Action::Action(const std::string& name, const std::string& url, ServiceType serviceType)
: m_Name(name)
, m_Url(url)
, m_ServiceType(serviceType)
{
    m_ActionDoc = UpnpMakeAction(name.c_str(), getServiceTypeUrn().c_str(), 0, nullptr);
}

void Action::addArgument(const std::string& name, const std::string& value)
{
    IXML_Document* pDoc = static_cast<IXML_Document*>(m_ActionDoc);
    if (UPNP_E_SUCCESS != UpnpAddToAction(&pDoc, m_Name.c_str(), getServiceTypeUrn().c_str(), name.c_str(), value.c_str()))
    {
        throw std::logic_error("Failed to add action to UPnP request: " + name);
    }
}

const xml::Document& Action::getActionDocument() const
{
    return m_ActionDoc;
}

std::string Action::getName() const
{
    return m_Name;
}

std::string Action::getUrl() const
{
    return m_Url;
}

std::string Action::getServiceTypeUrn() const
{
    return serviceTypeToUrnString(m_ServiceType);
}

ServiceType Action::getServiceType() const
{
    return m_ServiceType;
}

bool Action::operator==(const Action& other) const
{
    if (!m_ActionDoc && other.m_ActionDoc)
    {
        return false;
    }
    
    return m_ActionDoc.toString() == other.m_ActionDoc.toString();
}
    
}