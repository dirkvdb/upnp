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

#ifndef UPNP_ACTION_H
#define UPNP_ACTION_H

#include <string>
#include <vector>

#include <upnp.h>
#include <upnptools.h>

#include "upnp/upnptypes.h"
#include "upnp/upnpxmlutils.h"

namespace upnp
{

class Action
{
public:
    Action(const std::string& name, const std::string& url, ServiceType serviceType);

    void addArgument(const std::string& name, const std::string& value);

    const xml::Document& getActionDocument() const;

    std::string getName() const;
    std::string getUrl() const;
    std::string getServiceTypeUrn() const;
    ServiceType getServiceType() const;

    bool operator==(const Action& other) const;

private:
    std::string                 m_name;
    std::string                 m_url;
    ServiceType                 m_serviceType;

    xml::Document               m_actionDoc;
};

inline std::ostream& operator<< (std::ostream& os, const Action& action)
{
    return os << action.getActionDocument().toString();
}

}

#endif
