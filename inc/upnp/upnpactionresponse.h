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

#ifndef UPNP_ACTION_RESPONSE_H
#define UPNP_ACTION_RESPONSE_H

#include <string>
#include <vector>

#include "upnp/upnptypes.h"
#include "upnp/upnpxmlutils.h"

namespace upnp
{

class ActionResponse
{
public:
    ActionResponse(const std::string& name, ServiceType serviceType);
    ActionResponse(const ActionResponse&) = delete;
    ActionResponse(ActionResponse&& other) = default;

    void addArgument(const std::string& name, const std::string& value);

    const xml::Document& getActionDocument() const;

    std::string getName() const;
    std::string getServiceTypeUrn() const;
    ServiceType getServiceType() const;

    bool operator==(const ActionResponse& other) const;

private:
    std::string                 m_name;
    ServiceType                 m_serviceType;

    xml::Document               m_actionDoc;
};

inline std::ostream& operator<< (std::ostream& os, const ActionResponse& response)
{
    return os << response.getActionDocument().toString();
}

}

#endif
