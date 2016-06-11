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

class AVTransportServiceTest : public Test
{
public:
    AVTransportServiceTest()
    : service(rootDeviceMock, serviceImplMock)
    {
    }

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
    RootDeviceMock rootDeviceMock;
    AVTransportMock serviceImplMock;
    AVTransport::Service service;
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
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(AVTransportServiceTest, GetPositionInfo)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrack, "Track");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrackDuration, "00:01:00");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrackMetaData, "Meta");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrackURI, "http://track");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::RelativeTimePosition, "1");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::AbsoluteTimePosition, "2");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::RelativeCounterPosition, "3");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::AbsoluteCounterPosition, "4");

    auto a = createAction(AVTransport::Action::GetPositionInfo);
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::GetPositionInfo, a));
    
    ActionResponse expected(AVTransport::actionToString(AVTransport::Action::GetPositionInfo), serviceType);
    expected.addArgument("Track", "Track");
    expected.addArgument("TrackDuration", "00:01:00");
    expected.addArgument("TrackMetaData", "Meta");
    expected.addArgument("TrackURI", "http://track");
    expected.addArgument("RelTime", "1");
    expected.addArgument("AbsTime", "2");
    expected.addArgument("RelCount", "3");
    expected.addArgument("AbsCount", "4");
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(AVTransportServiceTest, GetMediaInfo)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::NumberOfTracks, "5");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentMediaDuration, "00:01:00");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrackURI, "Uri");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTrackMetaData, "UriMeta");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::NextAVTransportURI, "NextUri");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::NextAVTransportURIMetaData, "NextUriMeta");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::PlaybackStorageMedium, "Medium");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::RecordStorageMedium, "MediumRec");
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::RecordMediumWriteStatus, "Status");

    auto a = createAction(AVTransport::Action::GetMediaInfo);
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::GetMediaInfo, a));
    
    ActionResponse expected(AVTransport::actionToString(AVTransport::Action::GetMediaInfo), serviceType);
    expected.addArgument("NrTracks", "5");
    expected.addArgument("MediaDuration", "00:01:00");
    expected.addArgument("CurrentURI", "Uri");
    expected.addArgument("CurrentURIMetaData", "UriMeta");
    expected.addArgument("NextURI", "NextUri");
    expected.addArgument("NextURIMetaData", "NextUriMeta");
    expected.addArgument("PlayMedium", "Medium");
    expected.addArgument("RecordMedium", "MediumRec");
    expected.addArgument("WriteStatus", "Status");
    EXPECT_EQ(expected.toString(), response);
}

TEST_F(AVTransportServiceTest, GetCurrentTransportActions)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setInstanceVariable(s_connectionId, AVTransport::Variable::CurrentTransportActions, "Pause,Stop,Play");
    
    auto a = createAction(AVTransport::Action::GetCurrentTransportActions);
    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(AVTransport::Action::GetCurrentTransportActions, a));
    
    ActionResponse expected(AVTransport::actionToString(AVTransport::Action::GetCurrentTransportActions), serviceType);
    expected.addArgument("Actions", "Pause,Stop,Play");
    EXPECT_EQ(expected.toString(), response);
}

}
}
