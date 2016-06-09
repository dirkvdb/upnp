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
#include "upnp/upnp.avtransport.service.h"
#include "upnp.avtransport.typeconversions.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;

static const std::string s_controlUrl = "http://controlurl:80";
static const std::string s_eventNameSpaceId = "AVS";
static const uint32_t s_connectionId = 5;

struct RootDeviceMock : public IRootDevice
{
    MOCK_METHOD0(initialize, void());
    MOCK_METHOD0(uninitialize, void());

    MOCK_METHOD0(getWebrootUrl, std::string());
    MOCK_METHOD2(registerDevice, void(const std::string& deviceDescriptionXml, const Device& dev));

    MOCK_METHOD0(getUniqueDeviceName, std::string());

    MOCK_METHOD2(notifyEvent, void(const std::string& serviceId, std::string response));
    MOCK_METHOD3(addFileToHttpServer, void(const std::string& path, const std::string& contentType, const std::string& data));
    MOCK_METHOD3(addFileToHttpServer, void(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data));
    MOCK_METHOD1(removeFileFromHttpServer, void(const std::string& path));
};

struct AVTransportMock : public IAVTransport
{
    MOCK_METHOD3(setAVTransportURI, void(uint32_t instanceId, const std::string& uri, const std::string& metaData));
    MOCK_METHOD1(stop, void(uint32_t instanceId));
    MOCK_METHOD2(play, void(uint32_t instanceId, const std::string& speed));
    MOCK_METHOD3(seek, void(uint32_t instanceId, AVTransport::SeekMode mode, const std::string& target));
    MOCK_METHOD1(next, void(uint32_t instanceId));
    MOCK_METHOD1(previous, void(uint32_t instanceId));

    MOCK_METHOD3(setNextAVTransportURI, void(uint32_t instanceId, const std::string& /*uri*/, const std::string& /*metaData*/));
    MOCK_METHOD1(pause, void(uint32_t instanceId));
    MOCK_METHOD1(record, void(uint32_t instanceId));
    MOCK_METHOD2(setPlayMode, void(uint32_t instanceId, AVTransport::PlayMode));
    MOCK_METHOD2(setRecordQualityMode, void(uint32_t instanceId, const std::string& /*mode*/));
    MOCK_METHOD1(getCurrentTransportActions, std::vector<AVTransport::Action>(uint32_t instanceId));
};

template <typename ServiceType, typename VariableType, typename MockType>
class ServiceTestBase : public Test
{
protected:
    ServiceTestBase()
    : service(rootDeviceMock, serviceImplMock)
    {
    }

    RootDeviceMock rootDeviceMock;
    MockType serviceImplMock;
    ServiceType service;
};

class AVTransportServiceTest : public ServiceTestBase<AVTransport::Service, AVTransport::Variable, AVTransportMock>
{
public:
    Action createAction(AVTransport::Action type)
    {
        Action a(AVTransport::actionToString(type), s_controlUrl, serviceType);
        a.addArgument("InstanceID", std::to_string(s_connectionId));
        return a;
    }
    
    ActionRequest createActionRequest(AVTransport::Action type, const Action& a)
    {
        ActionRequest req;
        req.serviceType = serviceTypeToUrnIdString(serviceType);
        req.actionName = AVTransport::actionToString(type);
        req.action = a.toString();
        return req;
    }

    ServiceType serviceType = ServiceType{ServiceType::AVTransport, 1};
};

TEST_F(AVTransportServiceTest, Play)
{
    auto a = createAction(AVTransport::Action::Play);
    a.addArgument("Speed", "2");

    EXPECT_CALL(serviceImplMock, play(s_connectionId, "2"s));
    rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::Play, a));
}

TEST_F(AVTransportServiceTest, Stop)
{
    auto a = createAction(AVTransport::Action::Stop);

    EXPECT_CALL(serviceImplMock, stop(s_connectionId));
    rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::Stop, a));
}

TEST_F(AVTransportServiceTest, Pause)
{
    auto a = createAction(AVTransport::Action::Pause);

    EXPECT_CALL(serviceImplMock, pause(s_connectionId));
    rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::Pause, a));
}

TEST_F(AVTransportServiceTest, Previous)
{
    auto a = createAction(AVTransport::Action::Previous);

    EXPECT_CALL(serviceImplMock, previous(s_connectionId));
    rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::Previous, a));
}

TEST_F(AVTransportServiceTest, Next)
{
    auto a = createAction(AVTransport::Action::Next);

    EXPECT_CALL(serviceImplMock, next(s_connectionId));
    rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::Next, a));
}

TEST_F(AVTransportServiceTest, GetTransportInfo)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::TransportState, toString(AVTransport::State::Playing));
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::TransportStatus, toString(AVTransport::Status::Ok));
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::TransportPlaySpeed, "2");

    auto a = createAction(AVTransport::Action::GetTransportInfo);

    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::GetTransportInfo, a));
    
    ActionResponse expected(AVTransport::actionToString(AVTransport::Action::GetTransportInfo), serviceType);
    expected.addArgument("CurrentTransportState", toString(AVTransport::State::Playing));
    expected.addArgument("CurrentTransportStatus", toString(AVTransport::Status::Ok));
    expected.addArgument("CurrentSpeed", "2");
    EXPECT_EQ(response, expected.toString());
}

// TEST_F(AVTransportServiceTest, getPositionInfo)
// {
//     Action expectedAction("GetPositionInfo", s_controlUrl, serviceType());
//     expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

//     expectAction(expectedAction, { { "AbsCount",      "1" },
//                                    { "AbsTime",       "AbsTime" },
//                                    { "RelTime",       "RelTime" },
//                                    { "RelCount",      "2" },
//                                    { "Track",         "3" },
//                                    { "TrackDuration", "Duration" },
//                                    { "TrackMetaData", "Meta" },
//                                    { "TrackURI",      "URI"} } );

//     AVTransport::PositionInfo expectedInfoResponse;
//     expectedInfoResponse.absoluteCount = 1;
//     expectedInfoResponse.relativeCount = 2;
//     expectedInfoResponse.track = 3;
//     expectedInfoResponse.absoluteTime = "AbsTime";
//     expectedInfoResponse.relativeTime = "RelTime";
//     expectedInfoResponse.trackDuration = "Duration";
//     expectedInfoResponse.trackMetaData = "Meta";
//     expectedInfoResponse.trackURI = "URI";

//     EXPECT_CALL(statusMock, onStatus(Status(), expectedInfoResponse));
//     serviceInstance->getPositionInfo(s_connectionId, checkStatusCallback<AVTransport::PositionInfo>());
// }

// TEST_F(AVTransportServiceTest, getMediaInfo)
// {
//     Action expectedAction("GetMediaInfo", s_controlUrl, serviceType());
//     expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

//     expectAction(expectedAction, { { "NrTracks",            "5" },
//                                    { "MediaDuration",       "Duration" },
//                                    { "CurrentUri",          "Uri" },
//                                    { "CurrentUriMetaData",  "UriMeta" },
//                                    { "NextURI",             "NextUri" },
//                                    { "NextURIMetaData",     "NextUriMeta" },
//                                    { "PlayMedium",          "Medium" },
//                                    { "RecordMedium",        "MediumRec"},
//                                    { "WriteStatus",         "Status"} } );

//     AVTransport::MediaInfo expectedInfoResponse;
//     expectedInfoResponse.numberOfTracks = 5;
//     expectedInfoResponse.mediaDuration = "Duration";
//     expectedInfoResponse.currentURI = "Uri";
//     expectedInfoResponse.currentURIMetaData = "UriMeta";
//     expectedInfoResponse.nextURI = "NextUri";
//     expectedInfoResponse.nextURIMetaData = "NextUriMeta";
//     expectedInfoResponse.playMedium = "Medium";
//     expectedInfoResponse.recordMedium = "MediumRec";
//     expectedInfoResponse.writeStatus = "Status";

//     EXPECT_CALL(statusMock, onStatus(Status(), expectedInfoResponse));
//     serviceInstance->getMediaInfo(s_connectionId, checkStatusCallback<AVTransport::MediaInfo>());
// }

// TEST_F(AVTransportServiceTest, getCurrentTransportActions)
// {
//     Action expectedAction("GetCurrentTransportActions", s_controlUrl, serviceType());
//     expectedAction.addArgument("InstanceID", std::to_string(s_connectionId));

//     expectAction(expectedAction, { { "Actions", "Pause,Stop,Play" } } );

//     std::set<AVTransport::Action> expected = { AVTransport::Action::Play, AVTransport::Action::Pause, AVTransport::Action::Stop };
//     EXPECT_CALL(statusMock, onStatus(Status(), Matcher<std::set<AVTransport::Action>>(ContainerEq(expected))));
//     serviceInstance->getCurrentTransportActions(s_connectionId, checkStatusCallback<std::set<AVTransport::Action>>());
// }

}
}
