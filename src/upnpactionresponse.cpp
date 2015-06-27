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
#include "upnp/upnpactionresponse.h"

#include <stdexcept>

namespace upnp
{

ActionResponse::ActionResponse(const std::string& name, ServiceType serviceType)
: m_name(name)
, m_serviceType(serviceType)
, m_actionDoc(UpnpMakeActionResponse(name.c_str(), getServiceTypeUrn().c_str(), 0, nullptr), xml::Document::NoOwnership)
{
}

void ActionResponse::addArgument(const std::string& name, const std::string& value)
{
    IXML_Document* pDoc = static_cast<IXML_Document*>(m_actionDoc);
    auto rc = UpnpAddToActionResponse(&pDoc, m_name.c_str(), getServiceTypeUrn().c_str(), name.c_str(), value.c_str());
    if (UPNP_E_SUCCESS != rc)
    {
        throw Exception(rc, "Failed to add action to UPnP response: " + name);
    }
}

const xml::Document& ActionResponse::getActionDocument() const
{
    return m_actionDoc;
}

std::string ActionResponse::getName() const
{
    return m_name;
}

std::string ActionResponse::getServiceTypeUrn() const
{
    return serviceTypeToUrnTypeString(m_serviceType);
}

ServiceType ActionResponse::getServiceType() const
{
    return m_serviceType;
}

bool ActionResponse::operator==(const ActionResponse& other) const
{
    if (!m_actionDoc && other.m_actionDoc)
    {
        return false;
    }

    return m_actionDoc.toString() == other.m_actionDoc.toString();
}

}
