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
#include "gtest/gtest.h"

#include <iostream>
#include <algorithm>
#include <memory>

#include "upnpclientmock.h"
#include "eventlistenermock.h"
#include "testxmls.h"
#include "testutils.h"

#include "upnp/upnp.action.h"
#include "upnp/upnp.serviceclientbase.h"


using namespace utils;
using namespace testing;
using namespace std::placeholders;
using namespace std::chrono_literals;

namespace upnp
{
namespace test
{

static const std::string s_controlUrl               = "ControlUrl";
static const std::string s_subscriptionUrl          = "SubscriptionUrl";
static const std::string s_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string s_eventNameSpaceId         = "RCS";
static const std::string s_subscriptionId           = "subscriptionId";
static const auto g_defaultTimeout                  = 1801s;

enum class ServiceImplAction
{
    Action1,
    Action2
};

enum class ServiceImplVariable
{
    Var1,
    Var2
};

struct ServiceTraits
{
    using ActionType = ServiceImplAction;
    using VariableType = ServiceImplVariable;
    static ServiceType::Type SvcType;
    static constexpr uint8_t SvcVersion = 1;

    static ActionType actionFromString(const std::string& action)
    {
        if (action == "Action1")
        {
            return ActionType::Action1;
        }
        else if (action == "Action2")
        {
            return ActionType::Action2;
        }

        throw std::invalid_argument("invalid action");
    }

    static const char* actionToString(ActionType action)
    {
        switch (action)
        {
        case ActionType::Action1:   return "Action1";
        case ActionType::Action2:   return "Action2";
        default:
            throw std::invalid_argument("invalid action enum");
        }
    }

    static VariableType variableFromString(const std::string& var)
    {
        if (var == "Var1")
        {
            return VariableType::Var1;
        }
        else if (var == "Var2")
        {
            return VariableType::Var2;
        }

        throw std::invalid_argument("invalid variable");
    }

    static const char* variableToString(VariableType var)
    {
        switch (var)
        {
        case VariableType::Var1:   return "Var1";
        case VariableType::Var2:   return "Var2";
        default:
            throw std::invalid_argument("invalid variable enum");
        }
    }
};

class ServiceImplMock : public ServiceClientBase<ServiceTraits>
{
public:
    ServiceImplMock(IClient& client)
    : ServiceClientBase(client)
    {
    }

    MOCK_METHOD0(getSubscriptionTimeout, std::chrono::seconds());
};

class ServiceBaseTest : public Test
{
protected:
    ServiceBaseTest()
    : service(client)
    {
        Service serviceDesc;
        serviceDesc.type                  = { ServiceType::RenderingControl, 1 };
        serviceDesc.controlURL            = s_controlUrl;
        serviceDesc.eventSubscriptionURL  = s_subscriptionUrl;
        serviceDesc.scpdUrl               = s_serviceDescriptionUrl;

        auto device = std::make_shared<Device>();
        device->type = { DeviceType::MediaRenderer, 1 };
        device->services[serviceDesc.type.type] = serviceDesc;

        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(s_serviceDescriptionUrl))
            .WillOnce(Return(testxmls::simpleServiceDescription.c_str()));
        service->setDevice(device);

        Mock::VerifyAndClearExpectations(&client);
    }

    void subscribe()
    {
        EXPECT_CALL(*service, getSubscriptionTimeout()).WillOnce(Return(g_defaultTimeout));
        EXPECT_CALL(client, subscribeToService(s_subscriptionUrl, g_defaultTimeout, _))
            .WillOnce(Invoke([&] (const std::string&, int32_t, const std::shared_ptr<IServiceSubscriber>& cb) { subscriptionCallback = cb; }));

        service->StateVariableEvent.connect(std::bind(&ServiceImplMock::onStateVariableEvent, service.get(), _1, _2), service.get());
        service->subscribe();
        triggerSubscriptionComplete();
    }

    void unsubscribe()
    {
        service->StateVariableEvent.disconnect(service.get());
        service->unsubscribe();
    }

    void triggerSubscriptionComplete()
    {
        Upnp_Event_Subscribe event;
        event.ErrCode = UPNP_E_SUCCESS;
        strcpy(event.PublisherUrl, s_subscriptionUrl.c_str());
        strcpy(event.Sid, s_subscriptionId);

        subscriptionCallback->onServiceEvent(UPNP_EVENT_SUBSCRIBE_COMPLETE, &event);
    }

    void triggerLastChangeUpdate()
    {
        std::vector<testxmls::EventValue> ev = { { "Variable1", "VarValue"} };
        xml::Document doc(testxmls::generateStateVariableChangeEvent("Variable2", s_eventNameSpaceId, ev));

        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, s_subscriptionId);

        client.UPnPEventOccurredEvent(&event);
    }

    ClientMock client;
    ServiceImplMock service;
};

TEST_F(ServiceBaseTest, subscribe)
{
    // subscribe();

    // EXPECT_CALL(client, unsubscribeFromService(subscriptionCallback));
    // unsubscribe();
}

// TEST_F(ServiceBaseTest, renewSubscription)
// {
//     subscribe();

//     Upnp_Event_Subscribe event;
//     event.ErrCode = UPNP_E_SUCCESS;
//     strcpy(event.PublisherUrl, s_subscriptionUrl.c_str());
//     strcpy(event.Sid, s_subscriptionId);

//     int32_t timeout = g_defaultTimeout;
//     EXPECT_CALL(*service, getSubscriptionTimeout()).WillOnce(Return(g_defaultTimeout));
//     EXPECT_CALL(client, subscribeToService(s_subscriptionUrl, timeout)).WillOnce(Return(s_subscriptionId));
//     subscriptionCallback->onServiceEvent(UPNP_EVENT_SUBSCRIPTION_EXPIRED, &event);

//     EXPECT_CALL(client, unsubscribeFromService(subscriptionCallback));
//     unsubscribe();
// }

// TEST_F(ServiceBaseTest, unsubscribeNotSubscribed)
// {
//     EXPECT_CALL(client, unsubscribeFromService(subscriptionCallback)).Times(0);
//     unsubscribe();
// }

// TEST_F(ServiceBaseTest, executeAction)
// {
//     Action expectedAction("Action1", s_controlUrl, ServiceType::RenderingControl);
//     expectedAction.addArgument("Arg1", "1");
//     expectedAction.addArgument("Arg2", "2");

//     xml::Document expectedDoc("<doc></doc>");

//     EXPECT_CALL(*service, actionToString(ServiceImplAction::Action1)).WillOnce(Return("Action1"));
//     EXPECT_CALL(client, sendAction(expectedAction)).WillOnce(Return(expectedDoc));

//     auto doc = service->doExecuteAction(ServiceImplAction::Action1, { {"Arg1", "1"}, {"Arg2", "2"} });
//     EXPECT_EQ(expectedDoc.toString(), doc.toString());
// }

// TEST_F(ServiceBaseTest, stateVariableEvent)
// {
//     subscribe();

//     std::map<ServiceImplVariable, std::string> lastChange;
//     InSequence seq;
//     EXPECT_CALL(*service, variableFromString("Variable2")).WillOnce(Return(ServiceImplVariable::Var2));
//     EXPECT_CALL(*service, variableFromString("Variable1")).WillOnce(Return(ServiceImplVariable::Var1));
//     EXPECT_CALL(*service, handleStateVariableEvent(ServiceImplVariable::Var2, _));
//     EXPECT_CALL(*service, onStateVariableEvent(ServiceImplVariable::Var2, _)).WillOnce(SaveArg<1>(&lastChange));

//     triggerLastChangeUpdate();

//     EXPECT_EQ(1U, lastChange.size());
//     EXPECT_EQ("VarValue", lastChange[ServiceImplVariable::Var1]);

//     EXPECT_CALL(client, unsubscribeFromService(subscriptionCallback));
//     unsubscribe();
// }

}
}
