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

#include "utils/signal.h"
#include "utils/log.h"
#include "gtest/gtest.h"

#include <iostream>
#include <algorithm>
#include <memory>

#include "upnpclientmock.h"
#include "eventlistenermock.h"
#include "testxmls.h"
#include "testutils.h"

#include "upnp/upnpaction.h"
#include "upnp/upnpserviceclientbase.h"


using namespace utils;
using namespace testing;
using namespace std::placeholders;

#include "upnp/upnpxmlutils.h"

namespace upnp
{
namespace test
{

static const std::string g_controlUrl               = "ControlUrl";
static const std::string g_subscriptionUrl          = "SubscriptionUrl";
static const std::string g_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string g_eventNameSpaceId         = "RCS";
static const std::string g_connectionId             = "0";
static const uint32_t g_defaultTimeout              = 1801;
static const Upnp_SID g_subscriptionId              = "subscriptionId";

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

class ServiceImplMock : public ServiceClientBase<ServiceImplAction, ServiceImplVariable>
{
public:
    ServiceImplMock(IClient& client)
    : ServiceClientBase(client)
    {
    }
    
    virtual ~ServiceImplMock() {}

    MOCK_METHOD2(onStateVariableEvent, void(ServiceImplVariable, const std::map<ServiceImplVariable, std::string>&));

    MOCK_CONST_METHOD1(actionFromString, ServiceImplAction(const std::string&));
    MOCK_CONST_METHOD1(actionToString, std::string(ServiceImplAction));
    MOCK_CONST_METHOD1(variableFromString, ServiceImplVariable(const std::string&));
    MOCK_CONST_METHOD1(variableToString, std::string(ServiceImplVariable));
    
    MOCK_METHOD0(getType, ServiceType());
    MOCK_METHOD0(getSubscriptionTimeout, int32_t());
    MOCK_METHOD2(handleStateVariableEvent, void(ServiceImplVariable, const std::map<ServiceImplVariable, std::string>&));
    MOCK_METHOD1(handleUPnPResult, void(int));
    
    xml::Document doExecuteAction(ServiceImplAction actionType, const std::map<std::string, std::string>& args)
    {
        return executeAction(actionType, args);
    }
};

class ServiceBaseTest : public Test
{
public:
    virtual ~ServiceBaseTest() {}
    
protected:
    void SetUp()
    {
        service = std::make_unique<ServiceImplMock>(client);
        
        Service serviceDesc;
        serviceDesc.m_Type                  = ServiceType::RenderingControl;
        serviceDesc.m_ControlURL            = g_controlUrl;
        serviceDesc.m_EventSubscriptionURL  = g_subscriptionUrl;
        serviceDesc.m_SCPDUrl               = g_serviceDescriptionUrl;
        
        auto device = std::make_shared<Device>();
        device->m_Type = DeviceType::MediaRenderer;
        device->m_Services[serviceDesc.m_Type] = serviceDesc;
        
        ON_CALL(*service, getType()).WillByDefault(Return(ServiceType::RenderingControl));
        
        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(g_serviceDescriptionUrl))
            .WillOnce(Return(ixmlParseBuffer(testxmls::simpleServiceDescription.c_str())));
        EXPECT_CALL(*service, actionFromString("Action1")).WillOnce(Return(ServiceImplAction::Action1));
        EXPECT_CALL(*service, actionFromString("Action2")).WillOnce(Return(ServiceImplAction::Action2));
        service->setDevice(device);
        
        Mock::VerifyAndClearExpectations(&client);
    }
    
    void TearDown()
    {
    }
    
    void subscribe()
    {
        EXPECT_CALL(*service, getSubscriptionTimeout()).WillOnce(Return(g_defaultTimeout));
        EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, g_defaultTimeout, _))
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
        strcpy(event.PublisherUrl, g_subscriptionUrl.c_str());
        strcpy(event.Sid, g_subscriptionId);
        
        subscriptionCallback->onServiceEvent(UPNP_EVENT_SUBSCRIBE_COMPLETE, &event);
    }
    
    void triggerLastChangeUpdate()
    {
        std::vector<testxmls::EventValue> ev = { { "Variable1", "VarValue"} };
        xml::Document doc(testxmls::generateStateVariableChangeEvent("Variable2", g_eventNameSpaceId, ev));
        
        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);
        
        client.UPnPEventOccurredEvent(&event);
    }
    
    std::unique_ptr<ServiceImplMock>        service;
    StrictMock<ClientMock>                  client;
    
    std::shared_ptr<IServiceSubscriber>     subscriptionCallback;
};

TEST_F(ServiceBaseTest, subscribe)
{
    subscribe();
    
    EXPECT_CALL(client, unsubscribeFromService(g_subscriptionId));
    unsubscribe();
}

TEST_F(ServiceBaseTest, renewSubscription)
{
    subscribe();
    
    Upnp_Event_Subscribe event;
    event.ErrCode = UPNP_E_SUCCESS;
    strcpy(event.PublisherUrl, g_subscriptionUrl.c_str());
    strcpy(event.Sid, g_subscriptionId);
    
    int32_t timeout = g_defaultTimeout;
    EXPECT_CALL(*service, getSubscriptionTimeout()).WillOnce(Return(g_defaultTimeout));
    EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, timeout)).WillOnce(Return(g_subscriptionId));
    subscriptionCallback->onServiceEvent(UPNP_EVENT_SUBSCRIPTION_EXPIRED, &event);
    
    EXPECT_CALL(client, unsubscribeFromService(g_subscriptionId));
    unsubscribe();
}

TEST_F(ServiceBaseTest, unsubscribeNotSubscribed)
{
    EXPECT_CALL(client, unsubscribeFromService(_)).Times(0);
    unsubscribe();
}

TEST_F(ServiceBaseTest, executeAction)
{
    Action expectedAction("Action1", g_controlUrl, ServiceType::RenderingControl);
    expectedAction.addArgument("Arg1", "1");
    expectedAction.addArgument("Arg2", "2");

    xml::Document expectedDoc("<doc></doc>");

    EXPECT_CALL(*service, actionToString(ServiceImplAction::Action1)).WillOnce(Return("Action1"));
    EXPECT_CALL(client, sendAction(expectedAction)).WillOnce(Return(expectedDoc));
    
    auto doc = service->doExecuteAction(ServiceImplAction::Action1, { {"Arg1", "1"}, {"Arg2", "2"} });
    EXPECT_EQ(expectedDoc.toString(), doc.toString());
}

TEST_F(ServiceBaseTest, stateVariableEvent)
{
    subscribe();

    std::map<ServiceImplVariable, std::string> lastChange;
    InSequence seq;
    EXPECT_CALL(*service, variableFromString("Variable2")).WillOnce(Return(ServiceImplVariable::Var2));
    EXPECT_CALL(*service, variableFromString("Variable1")).WillOnce(Return(ServiceImplVariable::Var1));
    EXPECT_CALL(*service, handleStateVariableEvent(ServiceImplVariable::Var2, _));
    EXPECT_CALL(*service, onStateVariableEvent(ServiceImplVariable::Var2, _)).WillOnce(SaveArg<1>(&lastChange));
    
    triggerLastChangeUpdate();
    
    EXPECT_EQ(1U, lastChange.size());
    EXPECT_EQ("VarValue", lastChange[ServiceImplVariable::Var1]);
    
    EXPECT_CALL(client, unsubscribeFromService(g_subscriptionId));
    unsubscribe();
}

}
}
