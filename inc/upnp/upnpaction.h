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

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

namespace upnp
{
    
class Action
{
public:
    Action(const std::string& name, const std::string& serviceType);
    ~Action();
    
    void addArgument(const std::string& name, const std::string& value);
    IXML_Document* getActionDocument() const;
    
private:
    std::string         m_Name;
    std::string         m_ServiceType;
    IXML_Document*      m_pAction;
}; 
    
}

#endif
