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

#include "upnp/upnp.types.h"

namespace upnp
{

class Action
{
public:
    Action(const std::string& name, const std::string& url, ServiceType serviceType);
    Action(Action&&);
    Action& operator=(Action&&);
    ~Action();

    void addArgument(const std::string& name, const std::string& value);

    std::string toString() const;

    std::string getName() const;
    std::string getUrl() const;
    const char* getServiceTypeUrn() const;
    ServiceType getServiceType() const;

    bool operator==(const Action& other) const;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> m_pimpl;
};

inline std::ostream& operator<< (std::ostream& os, const Action& action)
{
    return os << action.toString();
}

}
