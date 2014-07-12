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
#include "upnpobjectprinter.h"

#include "upnp/upnpaction.h"
#include "upnp/upnpavtransportclient.h"
#include "upnp/upnpxmlutils.h"

using namespace utils;
using namespace testing;
using namespace std::placeholders;

namespace upnp
{
namespace test
{

static const std::string g_controlUrl               = "ControlUrl";
static const std::string g_subscriptionUrl          = "SubscriptionUrl";
static const std::string g_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string g_eventNameSpaceId         = "AVS";
static const uint32_t g_connectionId                = 0;
static const uint32_t g_defaultTimeout              = 1801;
static const Upnp_SID g_subscriptionId              = "subscriptionId";


class AVTransportTest : public Test
{
public:
    virtual ~AVTransportTest() {}
    
protected:
    void SetUp()
    {
        avtransport.reset(new AVTransport::Client(client));
        
        Service service;
        service.m_Type                  = ServiceType::AVTransport;
        service.m_ControlURL            = g_controlUrl;
        service.m_EventSubscriptionURL  = g_subscriptionUrl;
        service.m_SCPDUrl               = g_serviceDescriptionUrl;
        
        auto device = std::make_shared<Device>();
        device->m_Type = Device::Type::MediaRenderer;
        device->m_Services[service.m_Type] = service;
        
        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(g_serviceDescriptionUrl))
            .WillOnce(Return(xml::Document(testxmls::avtransportServiceDescription)));
        avtransport->setDevice(device);
        
        subscribe();
        
        Mock::VerifyAndClearExpectations(&client);
    }
    
    void TearDown()
    {
        Mock::VerifyAndClearExpectations(&client);

        unsubscribe();
        Mock::VerifyAndClearExpectations(&client);
        
        avtransport.reset();
    }
    
    void subscribe()
    {
        Upnp_FunPtr callback;
        void* pCookie = avtransport.get();
        
        EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, g_defaultTimeout, _, pCookie))
            .WillOnce(SaveArgPointee<2>(&callback));
        
        avtransport->StateVariableEvent.connect(std::bind(&EventListenerMock::AVTransportLastChangedEvent, &eventListener, _1, _2), this);
        avtransport->subscribe();
        
        Upnp_Event_Subscribe event;
        event.ErrCode = UPNP_E_SUCCESS;
        strcpy(event.PublisherUrl, g_subscriptionUrl.c_str());
        strcpy(event.Sid, g_subscriptionId);
        
        callback(UPNP_EVENT_SUBSCRIBE_COMPLETE, &event, pCookie);
    }
    
    void unsubscribe()
    {
        EXPECT_CALL(client, unsubscribeFromService(g_subscriptionId));
        
        avtransport->StateVariableEvent.disconnect(this);
        avtransport->unsubscribe();
    }
    
    void triggerLastChangeUpdate()
    {
        std::vector<testxmls::EventValue> ev = { { "TransportState", "PLAYING" },
                                                 { "TransportStatus", "OK" },
                                                 { "PlaybackStorageMedium", "NETWORK" },
                                                 { "CurrentTrackDuration", "01:05" },
                                                 { "AVTransportURI", "http://someurl.mp3" },
                                                 { "PossiblePlaybackStorageMedia", "media" },
                                                 { "CurrentPlayMode", "NORMAL" },
                                                 { "TransportPlaySpeed", "1"},
                                                 { "RecordMediumWriteStatus", "NOT_IMPLEMENTED" },
                                                 { "RecordStorageMedium", "NOT_IMPLEMENTED" },
                                                 { "CurrentRecordQualityMode", "NOT_IMPLEMENTED" },
                                                 { "PossibleRecordQualityModes", "NOT_IMPLEMENTED" },
                                                 { "NumberOfTracks", "1" },
                                                 { "CurrentTrack", "1" },
                                                 { "CurrentMediaDuration", "01:06" },
                                                 { "CurrentTrackMetaData", "Metadata" },
                                                 { "CurrentTrackURI", "http://trackurl.mp3" },
                                                 { "AVTransportURIMetaData", "AVmetadata" },
                                                 { "NextAVTransportURI", "NOT_IMPLEMENTED" },
                                                 { "NextAVTransportURIMetaData", "NOT_IMPLEMENTED" },
                                                 { "RelativeTimePosition", "00:30" },
                                                 { "AbsoluteTimePosition", "00:10" },
                                                 { "RelativeCounterPosition", "4" },
                                                 { "AbsoluteCounterPosition", "1" },
                                                 { "CurrentTransportActions", "Prev,Next,Stop" },
                                                 { "A_ARG_TYPE_SeekMode", "TRACK_NR" },
                                                 { "A_ARG_TYPE_SeekTarget", "target" },
                                                 { "A_ARG_TYPE_InstanceID", "InstanceId" } };
        
        xml::Document doc(testxmls::generateStateVariableChangeEvent("LastChange", g_eventNameSpaceId, ev));
        
        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);
        
        client.UPnPEventOccurredEvent(&event);
    }
    
    std::unique_ptr<AVTransport::Client>    avtransport;
    StrictMock<ClientMock>                  client;
    StrictMock<EventListenerMock>           eventListener;
};

TEST_F(AVTransportTest, supportedActions)
{
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetCurrentTransportActions));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetDeviceCapabilities));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetMediaInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetPositionInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetTransportInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::GetTransportSettings));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Next));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Pause));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Play));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Previous));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Seek));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::SetAVTransportURI));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::SetPlayMode));
    EXPECT_TRUE(avtransport->supportsAction(AVTransport::Action::Stop));
    
    EXPECT_FALSE(avtransport->supportsAction(AVTransport::Action::Record));
    EXPECT_FALSE(avtransport->supportsAction(AVTransport::Action::SetRecordQualityMode));
    EXPECT_FALSE(avtransport->supportsAction(AVTransport::Action::SetNextAVTransportURI));
}

TEST_F(AVTransportTest, lastChangeEvent)
{
    std::map<AVTransport::Variable, std::string> lastChange;
    EXPECT_CALL(eventListener, AVTransportLastChangedEvent(AVTransport::Variable::LastChange, _))
        .WillOnce(SaveArg<1>(&lastChange));
    
    triggerLastChangeUpdate();
    
    EXPECT_EQ("PLAYING",            lastChange[AVTransport::Variable::TransportState]);
    EXPECT_EQ("OK",                 lastChange[AVTransport::Variable::TransportStatus]);
    EXPECT_EQ("NETWORK",            lastChange[AVTransport::Variable::PlaybackStorageMedium]);
    EXPECT_EQ("01:05",              lastChange[AVTransport::Variable::CurrentTrackDuration]);
    EXPECT_EQ("http://someurl.mp3", lastChange[AVTransport::Variable::AVTransportURI]);
    EXPECT_EQ("media",              lastChange[AVTransport::Variable::PossiblePlaybackStorageMedia]);
    EXPECT_EQ("NORMAL",             lastChange[AVTransport::Variable::CurrentPlayMode]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::TransportPlaySpeed]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::RecordMediumWriteStatus]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::RecordStorageMedium]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::CurrentRecordQualityMode]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::PossibleRecordQualityModes]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::NumberOfTracks]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::CurrentTrack]);
    EXPECT_EQ("01:06",              lastChange[AVTransport::Variable::CurrentMediaDuration]);
    EXPECT_EQ("Metadata",           lastChange[AVTransport::Variable::CurrentTrackMetaData]);
    EXPECT_EQ("http://trackurl.mp3",lastChange[AVTransport::Variable::CurrentTrackURI]);
    EXPECT_EQ("AVmetadata",         lastChange[AVTransport::Variable::AVTransportURIMetaData]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::NextAVTransportURI]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransport::Variable::NextAVTransportURIMetaData]);
    EXPECT_EQ("00:30",              lastChange[AVTransport::Variable::RelativeTimePosition]);
    EXPECT_EQ("00:10",              lastChange[AVTransport::Variable::AbsoluteTimePosition]);
    EXPECT_EQ("4",                  lastChange[AVTransport::Variable::RelativeCounterPosition]);
    EXPECT_EQ("1",                  lastChange[AVTransport::Variable::AbsoluteCounterPosition]);
    EXPECT_EQ("Prev,Next,Stop",     lastChange[AVTransport::Variable::CurrentTransportActions]);
    EXPECT_EQ("TRACK_NR",           lastChange[AVTransport::Variable::ArgumentTypeSeekMode]);
    EXPECT_EQ("target",             lastChange[AVTransport::Variable::ArgumentTypeSeekTarget]);
    EXPECT_EQ("InstanceId",         lastChange[AVTransport::Variable::ArgumentTypeInstanceId]);
    
}

TEST_F(AVTransportTest, playDefaultSpeed)
{
    Action expectedAction("Play", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    expectedAction.addArgument("Speed", "1");
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->play(g_connectionId);
}

TEST_F(AVTransportTest, play)
{
    Action expectedAction("Play", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    expectedAction.addArgument("Speed", "2");
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->play(g_connectionId, "2");
}

TEST_F(AVTransportTest, stop)
{
    Action expectedAction("Stop", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->stop(g_connectionId);
}

TEST_F(AVTransportTest, pause)
{
    Action expectedAction("Pause", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->pause(g_connectionId);
}

TEST_F(AVTransportTest, previous)
{
    Action expectedAction("Previous", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->previous(g_connectionId);
}

TEST_F(AVTransportTest, next)
{
    Action expectedAction("Next", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->next(g_connectionId);
}

TEST_F(AVTransportTest, getTransportInfo)
{
    Action expectedAction("GetTransportInfo", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType(), {
                                                { "CurrentTransportState",  "PLAYING" },
                                                { "CurrentTransportStatus", "OK" },
                                                { "CurrentSpeed",           "Speed"} } )));
    
    AVTransport::TransportInfo info = avtransport->getTransportInfo(g_connectionId);
    EXPECT_EQ(AVTransport::State::Playing,      info.currentTransportState);
    EXPECT_EQ(AVTransport::Status::Ok,          info.currentTransportStatus);
    EXPECT_STREQ("Speed",                       info.currentSpeed.c_str());
}

TEST_F(AVTransportTest, getPositionInfo)
{
    Action expectedAction("GetPositionInfo", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType(), {
                                                { "AbsCount",      "1" },
                                                { "AbsTime",       "AbsTime" },
                                                { "RelTime",       "RelTime" },
                                                { "RelCount",      "2" },
                                                { "Track",         "3" },
                                                { "TrackDuration", "Duration" },
                                                { "TrackMetaData", "Meta" },
                                                { "TrackURI",      "URI"} } )));
    
    auto info = avtransport->getPositionInfo(g_connectionId);
    EXPECT_EQ(1, info.absoluteCount);
    EXPECT_EQ(2, info.relativeCount);
    EXPECT_EQ(3, info.track);
    EXPECT_EQ("AbsTime", info.absoluteTime);
    EXPECT_EQ("RelTime", info.relativeTime);
    EXPECT_EQ("Duration", info.trackDuration);
    EXPECT_EQ("Meta", info.trackMetaData);
    EXPECT_EQ("URI", info.trackURI);
}

}
}
