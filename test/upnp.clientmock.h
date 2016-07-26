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

#include <asio.hpp>

#include "gmock/gmock.h"
#include "upnp/upnp.clientinterface.h"

using namespace testing;

namespace upnp
{
namespace test
{

class ClientMock : public IClient
{
public:
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

    MOCK_METHOD1(dispatch, void(std::function<void()>));

    asio::io_service& ioService() noexcept override
    {
        return m_io;
    }

private:
    asio::io_service m_io;
};

}
}
