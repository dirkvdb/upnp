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

#include "upnp/upnpaction.h"
#include "upnp/upnpcontentdirectory.h"


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


class ContentDirectoryTest : public Test
{
public:
    virtual ~ContentDirectoryTest() {}
    
protected:
    void SetUp()
    {
        contentDirectory.reset(new ContentDirectory(client));
        
        Service service;
        service.m_Type = ServiceType::ContentDirectory;
        service.m_ControlURL = g_controlUrl;
        service.m_EventSubscriptionURL = g_subscriptionUrl;
        service.m_SCPDUrl = g_serviceDescriptionUrl;
        
        auto device = std::make_shared<Device>();
        device->m_Type = Device::Type::MediaServer;
        device->m_Services[service.m_Type] = service;
        
        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(g_serviceDescriptionUrl))
            .WillOnce(Return(xml::Document(testxmls::contentDirectoryServiceDescription.c_str())));
        EXPECT_CALL(client, sendAction(Action ("GetSearchCapabilities", g_controlUrl, ServiceType::ContentDirectory)))
            .WillOnce(Return(generateActionResponse("GetSearchCapabilities", ServiceType::ContentDirectory, { std::make_pair("SearchCaps", "dc:artist,dc:title") })));
        EXPECT_CALL(client, sendAction(Action ("GetSortCapabilities", g_controlUrl, ServiceType::ContentDirectory)))
            .WillOnce(Return(generateActionResponse("GetSortCapabilities", ServiceType::ContentDirectory, { std::make_pair("SortCaps", "dc:artist,dc:title") })));
        EXPECT_CALL(client, sendAction(Action ("GetSystemUpdateID", g_controlUrl, ServiceType::ContentDirectory)))
            .WillOnce(Return(generateActionResponse("GetSystemUpdateID", ServiceType::ContentDirectory, { std::make_pair("Id", "UpdateId") })));
        contentDirectory->setDevice(device);
        
        subscribe();
        
        Mock::VerifyAndClearExpectations(&client);
    }
    
    void TearDown()
    {
        Mock::VerifyAndClearExpectations(&client);
        unsubscribe();
        Mock::VerifyAndClearExpectations(&client);
    }
    
    void subscribe()
    {
        Upnp_FunPtr callback;
        void* pCookie = contentDirectory.get();
        
        EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, g_defaultTimeout, _, pCookie))
            .WillOnce(SaveArgPointee<2>(&callback));
        
        contentDirectory->LastChangedEvent.connect(std::bind(&EventListenerMock::ContentDirectoryLastChangedEvent, &eventListener, _1), this);
        contentDirectory->subscribe();
        
        Upnp_Event_Subscribe event;
        event.ErrCode = UPNP_E_SUCCESS;
        strcpy(event.PublisherUrl, g_subscriptionUrl.c_str());
        strcpy(event.Sid, g_subscriptionId);
        
        callback(UPNP_EVENT_SUBSCRIBE_COMPLETE, &event, pCookie);
    }
    
    void unsubscribe()
    {
        EXPECT_CALL(client, unsubscribeFromService(g_subscriptionId));
        
        contentDirectory->LastChangedEvent.disconnect(this);
        contentDirectory->unsubscribe();
    }
    
    void triggerLastChangeUpdate(const std::string& mute, const std::string& volume)
    {
        std::vector<testxmls::EventValue> ev = { { "PresetNameList", "FactoryDefaults, InstallationDefaults" },
            { "Mute", mute, {{ "channel", "master" }} },
            { "Volume", volume, {{ "channel", "master" }} } };
        
        xml::Document doc(testxmls::generateLastChangeEvent(g_eventNameSpaceId, ev));
        
        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);
        
        client.UPnPEventOccurredEvent(&event);
    }
    
    std::unique_ptr<ContentDirectory>       contentDirectory;
    StrictMock<ClientMock>                  client;
    StrictMock<EventListenerMock>           eventListener;
};

TEST_F(ContentDirectoryTest, getSortCapabilities)
{
    // see TEST errors
}

TEST_F(ContentDirectoryTest, getSearchCapabilities)
{
    // see TEST errors
}

TEST_F(ContentDirectoryTest, supportedActions)
{
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectoryAction::GetSearchCapabilities));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectoryAction::GetSortCapabilities));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectoryAction::GetSystemUpdateID));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectoryAction::Browse));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectoryAction::Search));
}

//TEST_F(RenderingControlTest, lastChangeEvent)
//{
//    std::map<RenderingControlVariable, std::string> lastChange;
//    EXPECT_CALL(eventListener, RenderingControlLastChangedEvent(_)).WillOnce(SaveArg<0>(&lastChange));
//    
//    triggerLastChangeUpdate("0", "35");
//    
//    EXPECT_EQ("FactoryDefaults, InstallationDefaults", lastChange[RenderingControlVariable::PresetNameList]);
//    EXPECT_EQ("0", lastChange[RenderingControlVariable::Mute]);
//    EXPECT_EQ("35", lastChange[RenderingControlVariable::Volume]);
//}

}
}
