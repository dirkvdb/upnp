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

#include "utils/log.h"
#include "utils/types.h"
#include "upnp/upnptypes.h"
#include "upnp/upnpactionresponse.h"
#include "upnp/upnprootdeviceinterface.h"
#include "upnp/upnpdeviceserviceexceptions.h"
#include "upnp/upnpservicevariable.h"

namespace upnp
{

template <typename VariableType>
class DeviceService
{
public:
    DeviceService(IRootDevice& dev, ServiceType type)
    : m_RootDevice(dev)
    , m_Type(type)
    {
        m_Variables.insert(std::make_pair(0, std::map<VariableType, ServiceVariable>()));
    }
    
    virtual ActionResponse onAction(const std::string& action, const xml::Document& request) = 0;
    virtual xml::Document getSubscriptionResponse() = 0;
    
    std::map<std::string, std::string> getVariables() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::map<std::string, std::string> vars;
        for (auto& var : m_Variables.at(0))
        {
            vars.insert(std::make_pair(variableToString(var.first), var.second));
        }
        
        return vars;
    }
    
    ServiceVariable getVariable(VariableType var) const
    {
        return getInstanceVariable(0, var);
    }
    
    ServiceVariable getInstanceVariable(uint32_t id, VariableType var) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto vars = m_Variables.find(id);
        if (vars != m_Variables.end())
        {
            auto v = vars->second.find(var);
            if (v != vars->second.end())
            {
                return v->second;
            }
        }
        
        return ServiceVariable();
    }
    
    void setVariable(VariableType var, const std::string& value)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Variables[0][var] = ServiceVariable(variableToString(var), value);
    }
    
    void setVariable(VariableType var, const std::string& value, const std::string& attrName, const std::string& attrValue)
    {
        ServiceVariable serviceVar(variableToString(var), value);
        serviceVar.addAttribute(attrName, attrValue);
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Variables[0][var] = serviceVar;
    }
    
    virtual void setInstanceVariable(uint32_t id, VariableType var, const std::string& value)
    {
        ServiceVariable serviceVar(variableToString(var), value);
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Variables[id][var] = serviceVar;
    }
    
    void setInstanceVariable(uint32_t id, VariableType var, const std::string& value, const std::string& attrName, const std::string& attrValue)
    {
        ServiceVariable serviceVar(variableToString(var), value);
        serviceVar.addAttribute(attrName, attrValue);
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Variables[id][var] = serviceVar;
    }
    
//    template <typename T>
//    typename std::enable_if<std::is_integral<T>::value, void>::type setVariable(VariableType var, const T& value)
//    {
//        m_Variables[0][var] = std::to_string(value);
//    }
//    
//    template <typename T>
//    typename std::enable_if<std::is_integral<T>::value, void>::type setInstanceVariable(uint32_t id, VariableType var, const T& value, const std::string& attrName, const std::string& attrValue)
//    {
//        ServiceVariable serviceVar(variableToString(var), value);
//        serviceVar.addAttribute(attrName, attrValue);
//        
//        m_Variables[id][var] = serviceVar;
//    }
    
protected:
    virtual std::string variableToString(VariableType type) const = 0;
    
    void notifyVariableChange(VariableType var, uint32_t instanceId)
    {
        const std::string ns = "urn:schemas-upnp-org:event-1-0";
    
        xml::Document doc;
        auto propertySet    = doc.createElement("e:propertyset");
        
        addPropertyToElement(instanceId, var, propertySet);
        
        doc.appendChild(propertySet);
    
        utils::log::debug("Variable change event: {}", doc.toString());
    
        m_RootDevice.notifyEvent(serviceTypeToUrnIdString(m_Type), doc);
    }
    
    void addPropertyToElement(int32_t instanceId, VariableType variable, xml::Element& elem)
    {
        auto doc    = elem.getOwnerDocument();
        auto prop   = doc.createElement("e:property");
        auto var    = doc.createElement(variableToString(variable));
        auto value  = doc.createNode(m_Variables.at(instanceId)[variable].getValue());

        var.appendChild(value);
        prop.appendChild(var);
        elem.appendChild(prop);
    }
    
    std::string vectorToCSV(const std::vector<std::string>& items)
    {
        std::stringstream ss;
        for (auto& item : items)
        {
            if (ss.tellp() > 0)
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
            if (ss.tellp() > 0)
            {
                ss << ',';
            }
        
            ss << toStringFunc(item);
        }
        
        return ss.str();
    }
    
    IRootDevice&                                                    m_RootDevice;
    ServiceType                                                     m_Type;
    std::map<uint32_t, std::map<VariableType, ServiceVariable>>     m_Variables;
    mutable std::mutex                                              m_Mutex;
};

}

#endif
