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
class ServiceClientBase
{
public:
    utils::Signal<VariableType, const std::map<VariableType, std::string>&> StateVariableEvent;

    ServiceClientBase(IClient& client)
    : m_client(client)
    {
    }
    
    virtual ~ServiceClientBase()
    {
        utils::log::debug("DEST {}", (void*)this);
        try
        {
            unsubscribe();
        }
        catch (std::exception& e)
        {
            utils::log::error(e.what());
        }
        utils::log::debug("RUCT {}", (void*) this);
    }
    
    virtual void setDevice(const std::shared_ptr<Device>& device)
    {
        if (device->implementsService(getType()))
        {
            m_service = device->m_Services[getType()];
            parseServiceDescription(m_service.m_SCPDUrl);
        }
    }
    
    void subscribe()
    {
        try { unsubscribe(); }
        catch (std::exception& e) { utils::log::warn(e.what()); }
        
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_subscriber = std::make_shared<ServiceSubscriber>(std::bind(&ServiceClientBase::eventCb, this, std::placeholders::_1, std::placeholders::_2));
        m_client.UPnPEventOccurredEvent.connect([this] (Upnp_Event* arg) { eventOccurred(arg); }, this);
        m_client.subscribeToService(m_service.m_EventSubscriptionURL, getSubscriptionTimeout(), m_subscriber);
    }
    
    void unsubscribe()
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        if (m_subscriber)
        {
            auto subCopy = m_subscriber;
            m_subscriber.reset();
            m_client.UPnPEventOccurredEvent.disconnect(this);
            m_client.unsubscribeFromService(subCopy);
        }
    }
    
    bool supportsAction(ActionType action) const
    {
        return m_supportedActions.find(action) != m_supportedActions.end();
    }
    
    virtual ActionType actionFromString(const std::string& action) const = 0;
    virtual std::string actionToString(ActionType action) const  = 0;
    virtual VariableType variableFromString(const std::string& var) const  = 0;
    virtual std::string variableToString(VariableType var) const  = 0;
    
protected:
    virtual void parseServiceDescription(const std::string& descriptionUrl)
    {
        xml::Document doc = m_client.downloadXmlDocument(descriptionUrl);

        try
        {
            for (auto& action : xml::utils::getActionsFromDescription(doc))
            {
                try
                {
                    m_supportedActions.insert(actionFromString(action));
                }
                catch (std::exception& e)
                {
                    utils::log::error(e.what());
                }
            }
            
            m_StateVariables = xml::utils::getStateVariablesFromDescription(doc);
        }
        catch (std::exception& e)
        {
            utils::log::error(e.what());
        }
    }
    
    void eventOccurred(Upnp_Event* pEvent)
    {
        std::string sid = pEvent->Sid;
        if (sid == m_subscriber->getSubscriptionId())
        {
            try
            {
                xml::Document doc(pEvent->ChangedVariables, xml::Document::NoOwnership);
                xml::Element propertySet = doc.getFirstChild();
                for (xml::Element property : propertySet.getChildNodes())
                {
                    for (xml::Element var : property.getChildNodes())
                    {
                        try
                        {
                            VariableType changedVar = variableFromString(var.getName());
                            
                            xml::Document changeDoc(var.getValue());
                            xml::Element eventNode = changeDoc.getFirstChild();
                            xml::Element instanceIDNode = eventNode.getChildElement("InstanceID");
                            
                            std::map<VariableType, std::string> vars;
                            for (xml::Element elem : instanceIDNode.getChildNodes())
                            {
                                auto str = elem.getAttribute("val");
                                utils::log::debug("{} {}", elem.getName(), elem.getAttribute("val"));
                                vars.insert(std::make_pair(variableFromString(elem.getName()), elem.getAttribute("val")));
                            }
                            
                            // let the service implementation process the event if necessary
                            handleStateVariableEvent(changedVar, vars);
                            
                            // notify clients
                            StateVariableEvent(changedVar, vars);
                        }
                        catch (std::exception& e)
                        {
                            utils::log::warn("Unknown event variable ignored: {}", e.what());
                            utils::log::debug(var.toString());
                        }
                    }
                }
            }
            catch (std::exception& e)
            {
                utils::log::error("Failed to parse event: {}", e.what());
            }
        }
    }
    
    xml::Document executeAction(ActionType actionType)
    {
        return executeAction(actionType, std::map<std::string, std::string> {});
    }
    
    xml::Document executeAction(ActionType actionType, const std::map<std::string, std::string>& args)
    {
        Action action(actionToString(actionType), m_service.m_ControlURL, getType());
        for (auto& arg : args)
        {
            action.addArgument(arg.first, arg.second);
        }
        
        try
        {
            return m_client.sendAction(action);
        }
        catch (Exception& e)
        {
            // give the service implementation a chance to translate the service specific error code
            handleUPnPResult(e.getErrorCode());
        }
        
        assert(false);
        return xml::Document();
    }
    
    virtual ServiceType getType() = 0;
    virtual int32_t getSubscriptionTimeout() = 0;
    virtual void handleStateVariableEvent(VariableType changedVariable, const std::map<VariableType, std::string>& variables) {}
    virtual void handleUPnPResult(int errorCode) = 0;

    std::vector<StateVariable>              m_StateVariables;
    
private:
    void eventCb(Upnp_EventType eventType, void* pEvent)
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        utils::log::debug("eventCb");
        switch (eventType)
        {
            case UPNP_EVENT_SUBSCRIBE_COMPLETE:
            {
                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
                if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
                {
                    utils::log::error("Error in Event Subscribe Callback: {} ({})", UpnpGetErrorMessage(pSubEvent->ErrCode), pSubEvent->ErrCode);
                }
                else
                {
                    m_subscriber->setSubscriptionId(pSubEvent->Sid);
                    
#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
                    utils::log::debug("Subscription complete: {}", m_SubscriptionId);
#endif
                }
                break;
            }
            case UPNP_EVENT_AUTORENEWAL_FAILED:
            case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
            {
                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
                
                try
                {
                    int32_t timeout = getSubscriptionTimeout();
                    m_subscriber->setSubscriptionId(m_client.subscribeToService(pSubEvent->PublisherUrl, timeout));

#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
                    utils::log::debug("Service subscription renewed: {}", m_SubscriptionId);
#endif
                }
                catch (std::exception& e)
                {
                    utils::log::error("Failed to renew event subscription: {}", e.what());
                }
                break;
            }
            case UPNP_EVENT_RENEWAL_COMPLETE:
#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
                utils::log::debug("Event subscription renewal complete");
#endif
                break;
            default:
                utils::log::info("Unhandled action: {}", eventType);
                break;
        }
    }

    class ServiceSubscriber : public IServiceSubscriber
    {
    public:
        ServiceSubscriber(std::function<void(Upnp_EventType, void*)> func)
        : m_cb(func)
        {
        }
    
        void onServiceEvent(Upnp_EventType eventType, void* pEvent) override
        {
            m_cb(eventType, pEvent);
        }
        
        void setSubscriptionId(const std::string& id)
        {
            m_subscriptionId = id;
        }
        
        std::string getSubscriptionId()
        {
            return m_subscriptionId;
        }
        
    private:
        std::function<void(Upnp_EventType, void*)>  m_cb;
        std::string                                 m_subscriptionId;
    };

    IClient&                                m_client;
    Service                                 m_service;
    std::set<ActionType>                    m_supportedActions;
    std::shared_ptr<ServiceSubscriber>      m_subscriber;
    std::mutex                              m_eventMutex;
};
    
}

#endif
