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

#include "upnp/upnp.clientinterface.h"
#include "upnp/upnp.action.h"
#include "upnp/upnpdevice.h"
#include "upnp/upnp.xml.parseutils.h"
#include "upnp/upnp.uv.h"

#include <set>
#include <map>
#include <vector>
#include <memory>

#include "rapidxml.hpp"

namespace upnp
{

template <typename ActionType, typename VariableType>
class ServiceClientBase
{
public:
    utils::Signal<VariableType, const std::map<VariableType, std::string>&> StateVariableEvent;

    ServiceClientBase(IClient2& client)
    : m_client(client)
    , m_subTimer(client.loop())
    {
    }

    virtual ~ServiceClientBase()
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

    virtual void setDevice(const std::shared_ptr<Device>& device)
    {
        if (device->implementsService(getType()))
        {
            m_service = device->m_services[getType()];
            parseServiceDescription(m_service.m_scpdUrl);
        }
    }

    void subscribe()
    {
        // try { unsubscribe(); }
        // catch (std::exception& e) { utils::log::warn(e.what()); }

        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_client.subscribeToService(m_service.m_eventSubscriptionURL, getSubscriptionTimeout(), [this] (int32_t status, const std::string& subId, std::chrono::seconds subTimeout) -> std::function<void(SubscriptionEvent)> {
            if (status < 0)
            {
                utils::log::error("Error subscribing to service");
                return nullptr;
            }

            m_subscriptionId = subId;
            m_subTimer.start(subTimeout / 2, subTimeout / 2, [] () {
                utils::log::warn("TODO: renew subscription");
            });

            return std::bind(&ServiceClientBase<ActionType, VariableType>::eventCb, this, std::placeholders::_1);
        });
    }

    void unsubscribe()
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_client.unsubscribeFromService(m_service.m_eventSubscriptionURL, m_subscriptionId, [] (int32_t status) {
            if (status < 0)
            {
                utils::log::debug("Unsubscribe from service failed");
            }
        });
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
        m_client.getFile(descriptionUrl, [this] (int32_t status, const std::string& contents) {
            if (status == 200)
            {
                using namespace rapidxml_ns;

                xml_document<> doc;
                doc.parse<parse_non_destructive>(contents.c_str());

                try
                {
                    for (auto& action : xml::getActionsFromDescription(doc))
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

                    m_StateVariables = xml::getStateVariablesFromDescription(doc);
                }
                catch (std::exception& e)
                {
                    utils::log::error(e.what());
                }
            }
        });
    }

//    void eventOccurred(Upnp_Event* pEvent)
//    {
//        std::string sid = pEvent->Sid;
//        if (sid == m_subscriber->getSubscriptionId())
//        {
//            try
//            {
//                xml::Document doc(pEvent->ChangedVariables, xml::Document::NoOwnership);
//                xml::Element propertySet = doc.getFirstChild();
//                for (xml::Element property : propertySet.getChildNodes())
//                {
//                    for (xml::Element var : property.getChildNodes())
//                    {
//                        try
//                        {
//                            VariableType changedVar = variableFromString(var.getName());
//
//                            xml::Document changeDoc(var.getValue());
//                            xml::Element eventNode = changeDoc.getFirstChild();
//                            xml::Element instanceIDNode = eventNode.getChildElement("InstanceID");
//
//                            std::map<VariableType, std::string> vars;
//                            for (xml::Element elem : instanceIDNode.getChildNodes())
//                            {
//                                auto str = elem.getAttribute("val");
//                                utils::log::debug("{} {}", elem.getName(), elem.getAttribute("val"));
//                                vars.insert(std::make_pair(variableFromString(elem.getName()), elem.getAttribute("val")));
//                            }
//
//                            // let the service implementation process the event if necessary
//                            handleStateVariableEvent(changedVar, vars);
//
//                            // notify clients
//                            StateVariableEvent(changedVar, vars);
//                        }
//                        catch (std::exception& e)
//                        {
//                            utils::log::warn("Unknown event variable ignored: {}", e.what());
//                            utils::log::debug(var.toString());
//                        }
//                    }
//                }
//            }
//            catch (std::exception& e)
//            {
//                utils::log::error("Failed to parse event: {}", e.what());
//            }
//        }
//    }

    void executeAction(ActionType actionType, std::function<void(int32_t, std::string)> cb)
    {
        executeAction(actionType, std::map<std::string, std::string> {}, std::move(cb));
    }

    void executeAction(ActionType actionType, const std::map<std::string, std::string>& args, std::function<void(int32_t, std::string)> cb)
    {
        Action2 action(actionToString(actionType), m_service.m_controlURL, getType());
        for (auto& arg : args)
        {
            action.addArgument(arg.first, arg.second);
        }

        m_client.sendAction(action, std::move(cb));
    }

    virtual ServiceType getType() = 0;
    virtual std::chrono::seconds getSubscriptionTimeout() = 0;
    virtual void handleStateVariableEvent(VariableType /*changedVariable*/, const std::map<VariableType, std::string>& /*variables*/) {}
    virtual void handleUPnPResult(int errorCode) = 0;

    std::vector<StateVariable> m_StateVariables;

private:
    void eventCb(const SubscriptionEvent& event)
    {
        utils::log::info(event.data);
    }

//    void eventCb(Upnp_EventType eventType, void* pEvent)
//    {
//        std::lock_guard<std::mutex> lock(m_eventMutex);
//        switch (eventType)
//        {
//            case UPNP_EVENT_SUBSCRIBE_COMPLETE:
//            {
//                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
//                if (pSubEvent->ErrCode != UPNP_E_SUCCESS)
//                {
//                    utils::log::error("Error in Event Subscribe Callback: {} ({})", UpnpGetErrorMessage(pSubEvent->ErrCode), pSubEvent->ErrCode);
//                }
//                else
//                {
//                    m_subscriber->setSubscriptionId(pSubEvent->Sid);
//
//#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
//                    utils::log::debug("Subscription complete: {}", m_SubscriptionId);
//#endif
//                }
//                break;
//            }
//            case UPNP_EVENT_AUTORENEWAL_FAILED:
//            case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
//            {
//                auto pSubEvent = reinterpret_cast<Upnp_Event_Subscribe*>(pEvent);
//
//                try
//                {
//                    int32_t timeout = getSubscriptionTimeout();
//                    m_subscriber->setSubscriptionId(m_client.subscribeToService(pSubEvent->PublisherUrl, timeout));
//
//#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
//                    utils::log::debug("Service subscription renewed: {}", m_SubscriptionId);
//#endif
//                }
//                catch (std::exception& e)
//                {
//                    utils::log::error("Failed to renew event subscription: {}", e.what());
//                }
//                break;
//            }
//            case UPNP_EVENT_RENEWAL_COMPLETE:
//#ifdef DEBUG_SERVICE_SUBSCRIPTIONS
//                utils::log::debug("Event subscription renewal complete");
//#endif
//                break;
//            default:
//                utils::log::info("Unhandled action: {}", eventType);
//                break;
//        }
//    }

    IClient2&                               m_client;
    Service                                 m_service;
    uv::Timer                               m_subTimer;
    std::set<ActionType>                    m_supportedActions;
    std::string                             m_subscriptionId;
    std::mutex                              m_eventMutex;
};

}

#endif
