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

#include "gmock/gmock.h"

#include "upnp/upnp.uv.h"
#include "upnp/upnpclientinterface.h"
#include "upnp/upnp.clientinterface.h"

using namespace testing;

namespace upnp
{
namespace test
{

class ClientMock : public IClient
{
public:
    MOCK_METHOD2(initialize, void(const char*, int32_t));
    MOCK_METHOD0(destroy, void());
    MOCK_METHOD0(reset, void());

    MOCK_CONST_METHOD0(getIpAddress, std::string());
    MOCK_CONST_METHOD0(getPort, int32_t());
    MOCK_CONST_METHOD2(searchDevicesOfType, void(DeviceType, int32_t));
    MOCK_CONST_METHOD1(searchAllDevices, void(int32_t));

    MOCK_CONST_METHOD2(subscribeToService, std::string(const std::string&, int32_t&));
    MOCK_CONST_METHOD1(unsubscribeFromService, void(const std::string&));
    MOCK_CONST_METHOD3(subscribeToService, void(const std::string&, int32_t, const std::shared_ptr<IServiceSubscriber>&));
    MOCK_CONST_METHOD1(unsubscribeFromService, void(const std::shared_ptr<IServiceSubscriber>&));
    MOCK_CONST_METHOD1(downloadXmlDocument, xml::Document(const std::string&));
};

class Client2Mock : public IClient2
{
public:
    Client2Mock()
    {
        EXPECT_CALL(*this, loop())
            .Times(AnyNumber())
            .WillRepeatedly(ReturnRef(m_loop));
    }

    MOCK_METHOD0(initialize, void());
    MOCK_METHOD2(initialize, void(const std::string&, uint16_t));
    MOCK_METHOD0(uninitialize, void());

    MOCK_CONST_METHOD0(getIpAddress, std::string());
    MOCK_CONST_METHOD0(getPort, uint16_t());

    MOCK_METHOD3(subscribeToService, void(const std::string& publisherUrl, std::chrono::seconds timeout, std::function<std::function<void(SubscriptionEvent)>(Status status, std::string subId, std::chrono::seconds timeout)>));
    MOCK_METHOD4(renewSubscription, void(const std::string& publisherUrl, const std::string& subscriptionId, std::chrono::seconds timeout, std::function<void(Status status, std::string subId, std::chrono::seconds timeout)>));
    MOCK_METHOD3(unsubscribeFromService, void(const std::string& publisherUrl, const std::string& subscriptionId, std::function<void(Status status)>));
    MOCK_METHOD2(sendAction, void(const Action&, std::function<void(Status status, std::string actionResult)>));
    MOCK_METHOD2(getFile, void(const std::string&, std::function<void(Status, std::string contents)>));

    MOCK_CONST_METHOD0(loop, uv::Loop&());

private:
    uv::Loop m_loop;
};

}
}
