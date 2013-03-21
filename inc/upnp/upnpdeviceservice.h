//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#ifndef UPNP_DEVICE_SERVICE_H
#define UPNP_DEVICE_SERVICE_H

#include <string>
#include <map>
#include <chrono>

#include "utils/types.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpactionresponse.h"
#include "upnp/upnpdeviceserviceexceptions.h"

namespace upnp
{

class DeviceService
{
public:
    DeviceService(ServiceType type);
    
    template <typename T>
    void SetVariable(const std::string& name, const T& value)
    {
        m_Variables[name] = std::to_string(value);
    }
    
    ServiceType getType() const;
    const std::map<std::string, std::string>& getVariables() const;
    
    virtual ActionResponse onAction(const std::string& action, const xml::Document& request) = 0;

protected:
    std::string vectorToCSV(const std::vector<std::string>& items)
    {
        std::stringstream ss;
        for (auto& item : items)
        {
            if (!ss.str().empty())
            {
                ss << ',';
            }
        
            ss << item;
        }
        
        return ss.str();
    }

    template <typename T>
    std::string vectorToCSV(const std::vector<T>& items, std::function<std::string(T)> toStringFunc)
    {
        std::stringstream ss;
        for (auto& item : items)
        {
            if (!ss.str().empty())
            {
                ss << ',';
            }
        
            ss << toStringFunc(item);
        }
        
        return ss.str();
    }

    ServiceType                             m_Type;
    std::map<std::string, std::string>      m_Variables;
};

}

#endif
