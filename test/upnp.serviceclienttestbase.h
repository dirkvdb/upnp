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

#include "utils/signal.h"
#include <gmock/gmock.h>

#include <memory>

#include "upnpclientmock.h"
#include "testxmls.h"
#include "testutils.h"
#include "upnpobjectprinter.h"
#include "eventlistenermock.h"

#include "upnp/upnp.action.h"
#include "upnp/upnpxmlutils.h"

namespace upnp
{

namespace test
{

using namespace utils;
using namespace testing;
using namespace std::placeholders;
using namespace std::chrono_literals;

static const std::string s_controlUrl               = "ControlUrl";
static const std::string s_subscriptionUrl          = "SubscriptionUrl";
static const std::string s_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string s_subscriptionId           = "subscriptionId";
static const uint32_t s_connectionId                = 0;
static const std::chrono::seconds s_defaultTimeout  = 1801s;

template <typename SvcType, typename StatusCbMock, typename VarType>
class ServiceClientTestBase : public Test
{
protected:
    ServiceClientTestBase(ServiceType type, const std::string& svcXml)
    : serviceType(type)
    , serviceXml(svcXml)
    {
    }

    virtual void SetUp() override
    {
        Service service;
        service.type                  = serviceType;
        service.controlURL            = s_controlUrl;
        service.eventSubscriptionURL  = s_subscriptionUrl;
        service.scpdUrl               = s_serviceDescriptionUrl;

        serviceInstance = std::make_unique<SvcType>(client);

        auto device = std::make_shared<Device>();
        device->type = DeviceType::MediaRenderer;
        device->services[service.type] = service;

        setDevice(device);
        subscribe();
    }

    virtual void TearDown() override
    {
        unsubscribe();
        Mock::VerifyAndClearExpectations(&client);

        serviceInstance.reset();
    }

    virtual void setDevice(std::shared_ptr<Device> device)
    {
        // set a valid device
        EXPECT_CALL(client, getFile(s_serviceDescriptionUrl, _)).WillOnce(InvokeArgument<1>(Status(), serviceXml));
        serviceInstance->setDevice(device, [] (Status status) {
            EXPECT_EQ(ErrorCode::Success, status.getErrorCode());
        });
    }

    void subscribe()
    {
        EXPECT_CALL(client, subscribeToService(s_subscriptionUrl, s_defaultTimeout, _))
            .WillOnce(WithArg<2>(Invoke([&] (auto& cb) {
                eventCb = cb(Status(), s_subscriptionId, s_defaultTimeout);
            })));

        serviceInstance->StateVariableEvent.connect(std::bind(&EventListenerMock<VarType>::LastChangedEvent, &eventListener, _1, _2), this);
        serviceInstance->subscribe([] (Status status) {
            EXPECT_EQ(ErrorCode::Success, status.getErrorCode());
        });
    }

    void unsubscribe()
    {
        EXPECT_CALL(client, unsubscribeFromService(s_subscriptionUrl, s_subscriptionId, _)).WillOnce(InvokeArgument<2>(Status()));

        serviceInstance->StateVariableEvent.disconnect(this);
        serviceInstance->unsubscribe([] (Status status) {
            EXPECT_EQ(ErrorCode::Success, status.getErrorCode());
        });
    }

    void expectAction(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&, responseVars] (auto& action, auto& cb) {
            EXPECT_EQ(expected.toString(), action.toString());
            cb(Status(), wrapSoap(generateActionResponse(expected.getName(), expected.getServiceType(), responseVars)));
        }));
    }

    std::function<void(Status)> checkStatusCallback()
    {
        return [this] (Status status) { statusMock.onStatus(status); };
    }

    template <typename T>
    std::function<void(Status, const T&)> checkStatusCallback()
    {
        return [this] (Status status, const T& arg) { statusMock.onStatus(status, arg); };
    }

    ServiceType                            serviceType;
    std::string                            serviceXml;
    std::unique_ptr<SvcType>               serviceInstance;
    std::function<void(SubscriptionEvent)> eventCb;

    StrictMock<Client2Mock>                client;
    StrictMock<EventListenerMock<VarType>> eventListener;
    StrictMock<StatusCbMock>               statusMock;
};

}
}
