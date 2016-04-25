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

static const std::string g_controlUrl               = "ControlUrl";
static const std::string g_subscriptionUrl          = "SubscriptionUrl";
static const std::string g_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string g_subscriptionId           = "subscriptionId";
static const uint32_t g_connectionId                = 0;
static const std::chrono::seconds g_defaultTimeout  = 1801s;

template <typename SvcType, typename StatusCbMock, typename VarType>
class ServiceTestBase : public Test
{
protected:
    ServiceTestBase(ServiceType type, const std::string& serviceXml)
    {
        Service service;
        service.m_type                  = type;
        service.m_controlURL            = g_controlUrl;
        service.m_eventSubscriptionURL  = g_subscriptionUrl;
        service.m_scpdUrl               = g_serviceDescriptionUrl;

        EXPECT_CALL(client, loop())
            .Times(AnyNumber())
            .WillRepeatedly(ReturnRef(loop));

        serviceInstance = std::make_unique<SvcType>(client);

        auto device = std::make_shared<Device>();
        device->m_type = DeviceType::MediaRenderer;
        device->m_services[service.m_type] = service;

        // set a valid device
        EXPECT_CALL(client, getFile(g_serviceDescriptionUrl, _)).WillOnce(InvokeArgument<1>(200, serviceXml));
        serviceInstance->setDevice(device);

        subscribe();

        Mock::VerifyAndClearExpectations(&client);
    }

    virtual ~ServiceTestBase()
    {
        Mock::VerifyAndClearExpectations(&client);

        unsubscribe();
        Mock::VerifyAndClearExpectations(&client);

        serviceInstance.reset();
    }

    void subscribe()
    {
        EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, g_defaultTimeout, _))
            .WillOnce(WithArg<2>(Invoke([&] (auto& cb) {
                eventCb = cb(200, g_subscriptionId, g_defaultTimeout);
            })));

        serviceInstance->StateVariableEvent.connect(std::bind(&EventListenerMock<VarType>::LastChangedEvent, &eventListener, _1, _2), this);
        serviceInstance->subscribe();
    }

    void unsubscribe()
    {
        EXPECT_CALL(client, unsubscribeFromService(g_subscriptionUrl, g_subscriptionId, _)).WillOnce(InvokeArgument<2>(200));

        serviceInstance->StateVariableEvent.disconnect(this);
        serviceInstance->unsubscribe();
    }

    void expectAction(const Action& expected, const std::vector<std::pair<std::string, std::string>>& responseVars = {})
    {
        EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&, responseVars] (auto& action, auto& cb) {
            EXPECT_EQ(expected.toString(), action.toString());
            cb(200, generateActionResponse(expected.getName(), expected.getServiceType(), responseVars));
        }));
    }

    std::function<void(int32_t)> checkStatusCallback()
    {
        return [this] (int32_t status) { statusMock.onStatus(status); };
    }

    template <typename T>
    std::function<void(int32_t, const T&)> checkStatusCallback()
    {
        return [this] (int32_t status, const T& arg) { statusMock.onStatus(status, arg); };
    }

    StrictMock<Client2Mock>                client;
    std::unique_ptr<SvcType>               serviceInstance;
    StrictMock<EventListenerMock<VarType>> eventListener;
    StrictMock<StatusCbMock>               statusMock;
    uv::Loop                               loop;

    std::function<void(SubscriptionEvent)> eventCb;
};

}
}
