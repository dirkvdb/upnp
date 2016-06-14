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

#include "gmock/gmock.h"

#include "upnp/upnp.action.h"
#include "upnp/upnp.connectionmanager.service.h"
#include "upnp.connectionmanager.typeconversions.h"
#include "upnp.rootdevicemock.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;

static const std::string s_controlUrl = "http://controlurl:80";
static const std::string s_eventNameSpaceId = "AVS";
static const uint32_t s_connectionId = 5;

struct ConnectionManagerMock : public IConnectionManager
{
    MOCK_METHOD2(prepareForConnection, void(const ProtocolInfo& protocolInfo, ConnectionManager::ConnectionInfo& info));
    MOCK_METHOD1(connectionComplete, void(int32_t connectionId));
    MOCK_METHOD1(getCurrentConnectionInfo, ConnectionManager::ConnectionInfo(int32_t connectionId));
};

class ConnectionManagerServiceTest : public Test
{
public:
    ConnectionManagerServiceTest()
    : service(rootDeviceMock, serviceImplMock)
    {
        rootDeviceMock.ControlActionRequested = [this] (auto& request) {
            return service.onAction(request.actionName, request.action).toString();
        };
    }

    Action createAction(ConnectionManager::Action type)
    {
        Action a(ConnectionManager::toString(type), s_controlUrl, serviceType);
        a.addArgument("InstanceID", std::to_string(s_connectionId));
        return a;
    }

    ActionRequest createActionRequest(ConnectionManager::Action type, const Action& a)
    {
        ActionRequest req;
        req.serviceType = serviceTypeToUrnIdString(serviceType);
        req.actionName = ConnectionManager::toString(type);
        req.action = a.toString();
        return req;
    }

    ServiceType serviceType = ServiceType{ServiceType::ConnectionManager, 1};
    RootDeviceMock rootDeviceMock;
    ConnectionManagerMock serviceImplMock;
    ConnectionManager::Service service;
};

TEST_F(ConnectionManagerServiceTest, GetProtocolInfo)
{
    ProtocolInfo sourceProtocolInfo("http-get:*:audio/mpeg:*");
    ProtocolInfo sinkProtocolInfo("http-get:*:*:*");

    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(0, ConnectionManager::Variable::SourceProtocolInfo, sourceProtocolInfo.toString());
    service.setInstanceVariable(0, ConnectionManager::Variable::SinkProtocolInfo, sinkProtocolInfo.toString());

    auto a = createAction(ConnectionManager::Action::GetProtocolInfo);
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(ConnectionManager::Action::GetProtocolInfo, a));

    ActionResponse expected(ConnectionManager::toString(ConnectionManager::Action::GetProtocolInfo), serviceType);
    expected.addArgument("Source", sourceProtocolInfo.toString());
    expected.addArgument("Sink", sinkProtocolInfo.toString());
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(ConnectionManagerServiceTest, PrepareForConnection)
{
    ProtocolInfo protocolInfo("http-get:*:audio/mpeg:*");

    auto a = createAction(ConnectionManager::Action::PrepareForConnection);
    a.addArgument("PeerConnectionManager", "1");
    a.addArgument("PeerConnectionID", "2");
    a.addArgument("Direction", "Input");
    a.addArgument("RemoteProtocolInfo", protocolInfo.toString());

    EXPECT_CALL(serviceImplMock, prepareForConnection(protocolInfo, _)).WillOnce(WithArg<1>(Invoke([] (auto& connInfo) {
        EXPECT_EQ("1"s, connInfo.peerConnectionManager);
        EXPECT_EQ(2, connInfo.peerConnectionId);
        EXPECT_EQ(ConnectionManager::Direction::Input, connInfo.direction);

        connInfo.connectionId = 3;
        connInfo.avTransportId = 4;
        connInfo.renderingControlServiceId = 5;
    })));

    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(ConnectionManager::Action::PrepareForConnection, a));

    ActionResponse expected(ConnectionManager::toString(ConnectionManager::Action::PrepareForConnection), serviceType);
    expected.addArgument("ConnectionID", "3");
    expected.addArgument("AVTransportID", "4");
    expected.addArgument("RcsID", "5");
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(ConnectionManagerServiceTest, ConnectionComplete)
{
    auto a = createAction(ConnectionManager::Action::ConnectionComplete);
    a.addArgument("ConnectionID", std::to_string(s_connectionId));

    EXPECT_CALL(serviceImplMock, connectionComplete(s_connectionId));
    rootDeviceMock.ControlActionRequested(createActionRequest(ConnectionManager::Action::ConnectionComplete, a));
}

TEST_F(ConnectionManagerServiceTest, GetCurrentConnectionIDs)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(0, ConnectionManager::Variable::CurrentConnectionIds, "1,3,5");

    auto a = createAction(ConnectionManager::Action::GetCurrentConnectionIDs);
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(ConnectionManager::Action::GetCurrentConnectionIDs, a));

    ActionResponse expected(ConnectionManager::toString(ConnectionManager::Action::GetCurrentConnectionIDs), serviceType);
    expected.addArgument("ConnectionIDs", "1,3,5");
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(ConnectionManagerServiceTest, GetCurrentConnectionInfo)
{
    auto a = createAction(ConnectionManager::Action::GetCurrentConnectionInfo);
    a.addArgument("ConnectionID", std::to_string(s_connectionId));

    ConnectionManager::ConnectionInfo connInfo;
    connInfo.renderingControlServiceId = 5;
    connInfo.avTransportId = 6;
    connInfo.protocolInfo = ProtocolInfo("http-get:*:audio/mpeg:*");
    connInfo.peerConnectionManager = "Peer";
    connInfo.peerConnectionId = 7;
    connInfo.direction = ConnectionManager::Direction::Input;
    connInfo.connectionStatus = ConnectionManager::ConnectionStatus::Ok;

    EXPECT_CALL(serviceImplMock, getCurrentConnectionInfo(s_connectionId)).WillOnce(Return(connInfo));
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(ConnectionManager::Action::GetCurrentConnectionInfo, a));

    ActionResponse expected(ConnectionManager::toString(ConnectionManager::Action::GetCurrentConnectionInfo), serviceType);
    expected.addArgument("RcsID", "5");
    expected.addArgument("AVTransportID", "6");
    expected.addArgument("ProtocolInfo", "http-get:*:audio/mpeg:*");
    expected.addArgument("PeerConnectionManager", "Peer");
    expected.addArgument("PeerConnectionID", "7");
    expected.addArgument("Direction", "Input");
    expected.addArgument("Status", "OK");
    EXPECT_EQ(expected.toString(), response);
}

}
}
