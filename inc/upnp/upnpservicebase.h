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

#ifndef UPNP_SERVICE_H
#define UPNP_SERVICE_H

#include "utils/log.h"
#include "utils/signal.h"

#include "upnp/upnpclientinterface.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnpxmlutils.h"

#include <upnp/upnp.h>

#include <set>
#include <map>
#include <vector>
#include <memory>

namespace upnp
{
    
class IClient;

template <typename ActionType, typename VariableType>
class ServiceBase
{
public:
    utils::Signal<void(const std::map<VariableType, std::string>&)> LastChangedEvent;

    ServiceBase(IClient& client)
    : m_Client(client)
    {
    }
    
    virtual ~ServiceBase()
    {
        try
        {
            unsubscribe();
        }
        catch (std::exception& e)
        {
            utils::log::error(e.what());
        }
    }
    
    void setDevice(const std::shared_ptr<Device>& device)
    {
        if (device->implementsService(getType()))
        {
            m_Service = device->m_Services[getType()];
            parseServiceDescription(m_Service.m_SCPDUrl);
        }
    }
    
    void subscribe()
    {
        unsubscribe();
        
        m_Client.UPnPEventOccurredEvent.connect(std::bind(&ServiceBase::eventOccurred, this, std::placeholders::_1), this);
        m_Client.subscribeToService(m_Service.m_EventSubscriptionURL, getSubscriptionTimeout(), &ServiceBase::eventCb, this);
    }
    
    void unsubscribe()
    {
        if (!m_SubscriptionId.empty())
        {
            m_Client.UPnPEventOccurredEvent.disconnect(this);
            m_Client.unsubscribeFromService(&(m_SubscriptionId[0]));
            m_SubscriptionId.clear();
        }
    }
    
    bool supportsAction(ActionType action) const
    {
        return m_SupportedActions.find(action) != m_SupportedActions.end();
    }
    
    virtual ActionType actionFromString(const std::string& action) = 0;
    virtual std::string actionToString(ActionType action) = 0;
    virtual VariableType variableFromString(const std::string& var) = 0;
    virtual std::string variableToString(VariableType var) = 0;
    
protected:
    virtual void parseServiceDescription(const std::string& descriptionUrl)
    {
        try
        {
            xml::Document doc = m_Client.downloadXmlDocument(descriptionUrl);
            for (auto& action : getActionsFromDescription(doc))
            {
                try
                {
                    m_SupportedActions.insert(actionFromString(action));
                }
                catch (std::exception& e)
                {
                    utils::log::error(e.what());
                }
            }
            
            m_StateVariables = getStateVariablesFromDescription(doc);
        }
        catch (std::exception& e)
        {
            utils::log::error(e.what());
        }
    }
    
    void eventOccurred(Upnp_Event* pEvent)
    {
        if (pEvent->Sid == m_SubscriptionId)
        {
            try
            {
                xml::Document doc(pEvent->ChangedVariables, xml::Document::NoOwnership);
                xml::Document changeDoc(doc.getChildElementValueRecursive("LastChange"));
                
                xml::Node instanceNode = changeDoc.getElementsByTagName("InstanceID").getNode(0);
                xml::NodeList children = instanceNode.getChildNodes();
                
                std::map<VariableType, std::string> vars;
                uint64_t numVars = children.size();
                for (uint64_t i = 0; i < numVars; ++i)
                {
                    try
                    {
                        xml::Element elem = children.getNode(i);
                        vars.insert(std::make_pair(variableFromString(elem.getName()), elem.getAttribute("val")));
                    }
                    catch (std::exception& e)
                    {
                        utils::log::warn("Unknown event variable ignored:", e.what());
                    }
                }
                
                // let the service implementation process the event if necessary
                handleLastChangeEvent(vars);
                
                // notify clients
                LastChangedEvent(vars);
            }
            catch (std::exception& e)
            {
                utils::log::error("Failed to parse event:", e.what());
            }
        }
    }
    
    xml::Document executeAction(ActionType actionType)
    {
        return executeAction(actionType, {});
    }
    
    xml::Document executeAction(ActionType actionType, const std::map<std::string, std::string>& args)
    {
        Action action(actionToString(actionType), m_Service.m_ControlURL, getType());
        for (auto& arg : args)
        {
            action.addArgument(arg.first, arg.second);
        }
        
        try
        {
            return m_Client.sendAction(action);
        }
        catch (UPnPException& e)
        {
            handleUPnPResult(e.getErrorCode());
        }
        
        assert(false);
        return xml::Document();
    }
    
    static int eventCb(Upnp_EventType eventType, void* pEvent, void* pInstance)
    {
        auto rc = reinterpret_cast<ServiceBase<ActionType, VariableType>*>(pInstance);
        
        switch (eventType)
        {
            case UPNP_EVENT_SUBSCRIBE_COMPLETE:
            {
                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
                if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
                {
                    utils::log::error("Error in Event Subscribe Callback:", pSubEvent->ErrCode);
                }
                else
                {
                    if (pSubEvent->Sid)
                    {
                        rc->m_SubscriptionId = pSubEvent->Sid;
                    }
                    else
                    {
                        rc->m_SubscriptionId.clear();
                        utils::log::error("Subscription id for device is empty");
                    }
                }
                break;
            }
            case UPNP_EVENT_AUTORENEWAL_FAILED:
            case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
            {
                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
                
                try
                {
                    int32_t timeout = rc->getSubscriptionTimeout();
                    rc->m_SubscriptionId = rc->m_Client.subscribeToService(pSubEvent->PublisherUrl, timeout);
                    
                    utils::log::debug("Service subscription renewed: ", rc->m_SubscriptionId);
                }
                catch (std::exception& e)
                {
                    utils::log::error(std::string("Failed to renew event subscription: ") + e.what());
                }
                break;
            }
            case UPNP_EVENT_RENEWAL_COMPLETE:
                utils::log::debug("Event subscription renewal complete");
                break;
            default:
                utils::log::info("Unhandled action:", eventType);
                break;
        }
        
        return 0;
    }
    
    virtual ServiceType getType() = 0;
    virtual int32_t getSubscriptionTimeout() = 0;
    virtual void handleLastChangeEvent(const std::map<VariableType, std::string>& variables) {}
    virtual void handleUPnPResult(int errorCode) = 0;
    
    std::vector<StateVariable>              m_StateVariables;
    
private:
    IClient&                                m_Client;
    Service                                 m_Service;
    std::set<ActionType>                    m_SupportedActions;
    std::string                             m_SubscriptionId;
};
    
}

#endif
