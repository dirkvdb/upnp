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

#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>
#include <memory>

#include "upnpclientmock.h"
#include "eventlistenermock.h"
#include "testxmls.h"
#include "testutils.h"
#include "upnpobjectprinter.h"

#include "upnp/upnpaction.h"
#include "upnp/upnpavtransport.h"
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
static const std::string g_connectionId             = "0";
static const uint32_t g_defaultTimeout              = 1801;
static const Upnp_SID g_subscriptionId              = "subscriptionId";


class AVTransportTest : public Test
{
public:
    virtual ~AVTransportTest() {}
    
protected:
    void SetUp()
    {
        avtransport.reset(new AVTransport(client));
        
        Service service;
        service.m_Type                  = ServiceType::AVTransport;
        service.m_ControlURL            = g_controlUrl;
        service.m_EventSubscriptionURL  = g_subscriptionUrl;bool supportsAction(Action action);
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
        
        avtransport->LastChangedEvent.connect(std::bind(&EventListenerMock::AVTransportLastChangedEvent, &eventListener, _1), this);
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
        
        avtransport->LastChangedEvent.disconnect(this);
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
        
        xml::Document doc(testxmls::generateLastChangeEvent(g_eventNameSpaceId, ev));
        
        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);
        
        client.UPnPEventOccurredEvent(&event);
    }
    
    std::unique_ptr<AVTransport>            avtransport;
    StrictMock<ClientMock>                  client;
    StrictMock<EventListenerMock>           eventListener;
};

TEST_F(AVTransportTest, supportedActions)
{
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetCurrentTransportActions));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetDeviceCapabilities));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetMediaInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetPositionInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetTransportInfo));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::GetTransportSettings));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Next));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Pause));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Play));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Previous));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Seek));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::SetAVTransportURI));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::SetPlayMode));
    EXPECT_TRUE(avtransport->supportsAction(AVTransportAction::Stop));
    
    EXPECT_FALSE(avtransport->supportsAction(AVTransportAction::Record));
    EXPECT_FALSE(avtransport->supportsAction(AVTransportAction::SetRecordQualityMode));
    EXPECT_FALSE(avtransport->supportsAction(AVTransportAction::SetNextAVTransportURI));
}

TEST_F(AVTransportTest, lastChangeEvent)
{
    std::map<AVTransportVariable, std::string> lastChange;
    EXPECT_CALL(eventListener, AVTransportLastChangedEvent(_)).WillOnce(SaveArg<0>(&lastChange));
    
    triggerLastChangeUpdate();
    
    EXPECT_EQ("PLAYING",            lastChange[AVTransportVariable::TransportState]);
    EXPECT_EQ("OK",                 lastChange[AVTransportVariable::TransportStatus]);
    EXPECT_EQ("NETWORK",            lastChange[AVTransportVariable::PlaybackStorageMedium]);
    EXPECT_EQ("01:05",              lastChange[AVTransportVariable::CurrentTrackDuration]);
    EXPECT_EQ("http://someurl.mp3", lastChange[AVTransportVariable::AVTransportURI]);
    EXPECT_EQ("media",              lastChange[AVTransportVariable::PossiblePlaybackStorageMedia]);
    EXPECT_EQ("NORMAL",             lastChange[AVTransportVariable::CurrentPlayMode]);
    EXPECT_EQ("1",                  lastChange[AVTransportVariable::TransportPlaySpeed]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::RecordMediumWriteStatus]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::RecordStorageMedium]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::CurrentRecordQualityMode]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::PossibleRecordQualityModes]);
    EXPECT_EQ("1",                  lastChange[AVTransportVariable::NumberOfTracks]);
    EXPECT_EQ("1",                  lastChange[AVTransportVariable::CurrentTrack]);
    EXPECT_EQ("01:06",              lastChange[AVTransportVariable::CurrentMediaDuration]);
    EXPECT_EQ("Metadata",           lastChange[AVTransportVariable::CurrentTrackMetaData]);
    EXPECT_EQ("http://trackurl.mp3",lastChange[AVTransportVariable::CurrentTrackURI]);
    EXPECT_EQ("AVmetadata",         lastChange[AVTransportVariable::AVTransportURIMetaData]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::NextAVTransportURI]);
    EXPECT_EQ("NOT_IMPLEMENTED",    lastChange[AVTransportVariable::NextAVTransportURIMetaData]);
    EXPECT_EQ("00:30",              lastChange[AVTransportVariable::RelativeTimePosition]);
    EXPECT_EQ("00:10",              lastChange[AVTransportVariable::AbsoluteTimePosition]);
    EXPECT_EQ("4",                  lastChange[AVTransportVariable::RelativeCounterPosition]);
    EXPECT_EQ("1",                  lastChange[AVTransportVariable::AbsoluteCounterPosition]);
    EXPECT_EQ("Prev,Next,Stop",     lastChange[AVTransportVariable::CurrentTransportActions]);
    EXPECT_EQ("TRACK_NR",           lastChange[AVTransportVariable::ArgumentTypeSeekMode]);
    EXPECT_EQ("target",             lastChange[AVTransportVariable::ArgumentTypeSeekTarget]);
    EXPECT_EQ("InstanceId",         lastChange[AVTransportVariable::ArgumentTypeInstanceId]);
    
}

TEST_F(AVTransportTest, playDefaultSpeed)
{
    Action expectedAction("Play", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    expectedAction.addArgument("Speed", "1");
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->play(g_connectionId);
}

TEST_F(AVTransportTest, play)
{
    Action expectedAction("Play", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    expectedAction.addArgument("Speed", "2");
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->play(g_connectionId, "2");
}

TEST_F(AVTransportTest, stop)
{
    Action expectedAction("Stop", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->stop(g_connectionId);
}

TEST_F(AVTransportTest, pause)
{
    Action expectedAction("Pause", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->pause(g_connectionId);
}

TEST_F(AVTransportTest, previous)
{
    Action expectedAction("Previous", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->previous(g_connectionId);
}

TEST_F(AVTransportTest, next)
{
    Action expectedAction("Next", g_controlUrl, ServiceType::AVTransport);
    expectedAction.addArgument("InstanceID", g_connectionId);
    
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));
    
    avtransport->next(g_connectionId);
}

}
}
