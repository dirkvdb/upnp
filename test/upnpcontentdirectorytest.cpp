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
#include "utils/timeoperations.h"

#include "gtest/gtest.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>

#include "upnpclientmock.h"
#include "eventlistenermock.h"
#include "testxmls.h"
#include "testutils.h"

#include "upnp/upnpaction.h"
#include "upnp/upnpcontentdirectoryclient.h"


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

class ItemSubscriber
{
public:
    MOCK_METHOD1(onItem, void(const std::shared_ptr<Item>&));
};


class ContentDirectoryTest : public Test
{
public:
    virtual ~ContentDirectoryTest() {}
    
protected:
    void SetUp()
    {
        contentDirectory = std::make_unique<ContentDirectory::Client>(client);
        
        Service service;
        service.m_Type = ServiceType::ContentDirectory;
        service.m_ControlURL = g_controlUrl;
        service.m_EventSubscriptionURL = g_subscriptionUrl;
        service.m_SCPDUrl = g_serviceDescriptionUrl;
        
        auto device = std::make_shared<Device>();
        device->m_Type = DeviceType::MediaServer;
        device->m_Services[service.m_Type] = service;
        
        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(g_serviceDescriptionUrl))
            .WillOnce(Return(xml::Document(testxmls::contentDirectoryServiceDescription.c_str())));
        EXPECT_CALL(client, sendAction(Action ("GetSearchCapabilities", g_controlUrl, ServiceType::ContentDirectory)))
            .WillOnce(Return(generateActionResponse("GetSearchCapabilities", ServiceType::ContentDirectory, { std::make_pair("SearchCaps", "upnp:artist,dc:title") })));
        EXPECT_CALL(client, sendAction(Action ("GetSortCapabilities", g_controlUrl, ServiceType::ContentDirectory)))
            .WillOnce(Return(generateActionResponse("GetSortCapabilities", ServiceType::ContentDirectory, { std::make_pair("SortCaps", "upnp:artist,dc:title,upnp:genre") })));
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
        
        contentDirectory->StateVariableEvent.connect(std::bind(&EventListenerMock::ContentDirectoryLastChangedEvent, &eventListener, _1, _2), this);
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
        
        contentDirectory->StateVariableEvent.disconnect(this);
        contentDirectory->unsubscribe();
    }
    
    void triggerLastChangeUpdate(const std::string& mute, const std::string& volume)
    {
        std::vector<testxmls::EventValue> ev = { { "PresetNameList", "FactoryDefaults, InstallationDefaults" },
                                                 { "Mute", mute, {{ "channel", "master" }} },
                                                 { "Volume", volume, {{ "channel", "master" }} } };
        
        xml::Document doc(testxmls::generateStateVariableChangeEvent("LastChange", g_eventNameSpaceId, ev));
        
        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);
        
        client.UPnPEventOccurredEvent(&event);
    }
    
    std::string getIndexString(uint32_t index)
    {
        std::stringstream ss;
        ss << std::setw(6) << std::setfill('0') << index;
        
        return ss.str();
    }
    
    std::vector<Item> generateContainers(uint32_t count, const std::string& upnpClass)
    {
        std::vector<Item> containers;
        
        for (uint32_t i = 0; i < count; ++i)
        {
            std::string index = getIndexString(i);
            Item container("Id" + index, "Title" + index);
            container.setParentId("ParentId");
            container.setChildCount(i);
            
            container.addMetaData(Property::Creator,        "Creator" + index);
            container.addMetaData(Property::AlbumArt,       "AlbumArt" + index);
            container.addMetaData(Property::Class,          upnpClass);
            container.addMetaData(Property::Genre,          "Genre" + index);
            container.addMetaData(Property::Artist,         "Artist" + index);
            
            containers.push_back(container);
        }
        
        return containers;
    }
    
    std::vector<Item> generateItems(uint32_t count, const std::string& upnpClass)
    {
        std::vector<Item> items;
    
        for (uint32_t i = 0; i < count; ++i)
        {
            std::string index = getIndexString(i);
            Item item("Id" + index, "Title" + index);
            item.setParentId("ParentId");
            
            item.addMetaData(Property::Actor,           "Actor" + index);
            item.addMetaData(Property::Album,           "Album" + index);
            item.addMetaData(Property::AlbumArt,        "AlbumArt" + index);
            item.addMetaData(Property::Class,           upnpClass);
            item.addMetaData(Property::Genre,           "Genre" + index);
            item.addMetaData(Property::Description,     "Description" + index);
            item.addMetaData(Property::Date,            "01/01/196" + index);
            item.addMetaData(Property::TrackNumber,     index);
            
            items.push_back(item);
        }
        
        return items;
    }
    
    std::unique_ptr<ContentDirectory::Client>   contentDirectory;
    StrictMock<ClientMock>                      client;
    StrictMock<EventListenerMock>               eventListener;
};

TEST_F(ContentDirectoryTest, getSearchCapabilities)
{
    auto props = contentDirectory->getSearchCapabilities();

    EXPECT_EQ(2U, props.size());
    EXPECT_NE(props.end(), std::find(props.begin(), props.end(), Property::Artist));
    EXPECT_NE(props.end(), std::find(props.begin(), props.end(), Property::Title));
}

TEST_F(ContentDirectoryTest, getSortCapabilities)
{
    auto props = contentDirectory->getSortCapabilities();
    
    EXPECT_EQ(3U, props.size());
    EXPECT_NE(props.end(), std::find(props.begin(), props.end(), Property::Artist));
    EXPECT_NE(props.end(), std::find(props.begin(), props.end(), Property::Title));
    EXPECT_NE(props.end(), std::find(props.begin(), props.end(), Property::Genre));
}

TEST_F(ContentDirectoryTest, supportedActions)
{
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectory::Action::GetSearchCapabilities));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectory::Action::GetSortCapabilities));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectory::Action::GetSystemUpdateID));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectory::Action::Browse));
    EXPECT_TRUE(contentDirectory->supportsAction(ContentDirectory::Action::Search));
}

TEST_F(ContentDirectoryTest, browseAction)
{
    const uint32_t size = 10;

    Action expectedAction("Browse", g_controlUrl, ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "100");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");
    
    InSequence seq;
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem"))));

    auto result = contentDirectory->browseDirectChildren(ContentDirectory::Client::All, "ObjectId", "filter", 0, 100, "sort");
    EXPECT_EQ(size, result.totalMatches);
    EXPECT_EQ(size, result.numberReturned);
    
    uint32_t containerCount = 0;
    uint32_t itemCount = 0;
    for (auto& item : result.result)
    {
        if (item->getClass() == Class::Container)
        {
            std::string index = getIndexString(containerCount);
            EXPECT_EQ("Id" + index,             item->getObjectId());
            EXPECT_EQ("ParentId",               item->getParentId());
            EXPECT_EQ("Title" + index,          item->getTitle());
            EXPECT_EQ(containerCount,           item->getChildCount());
            
            EXPECT_EQ("Artist" + index,         item->getMetaData(Property::Artist));
            EXPECT_EQ("Creator" + index,        item->getMetaData(Property::Creator));
            EXPECT_EQ("AlbumArt" + index,       item->getMetaData(Property::AlbumArt));
            EXPECT_EQ("Genre" + index,          item->getMetaData(Property::Genre));
            
            EXPECT_TRUE(item->getMetaData(Property::Actor).empty());
            EXPECT_TRUE(item->getMetaData(Property::Album).empty());
            EXPECT_TRUE(item->getMetaData(Property::Description).empty());
            EXPECT_TRUE(item->getMetaData(Property::Date).empty());
            EXPECT_TRUE(item->getMetaData(Property::TrackNumber).empty());
            
            ++containerCount;
        }
        else if (item->getClass() == Class::Audio)
        {
            std::string index = getIndexString(itemCount);
            EXPECT_EQ("Id" + index,             item->getObjectId());
            EXPECT_EQ("ParentId",               item->getParentId());
            EXPECT_EQ("Title" + index,          item->getTitle());
            EXPECT_EQ(0U,                       item->getChildCount());
            
            EXPECT_EQ("Actor" + index,          item->getMetaData(Property::Actor));
            EXPECT_EQ("Album" + index,          item->getMetaData(Property::Album));
            EXPECT_EQ("AlbumArt" + index,       item->getMetaData(Property::AlbumArt));
            EXPECT_EQ("Genre" + index,          item->getMetaData(Property::Genre));
            EXPECT_EQ("Description" + index,    item->getMetaData(Property::Description));
            EXPECT_EQ("01/01/196" + index,      item->getMetaData(Property::Date));
            EXPECT_EQ(index,                    item->getMetaData(Property::TrackNumber));
            
            ++itemCount;
        }
        else
        {
            FAIL() << "Unexpected class type encountered";
        }
    }
}

TEST_F(ContentDirectoryTest, DISABLED_performanceTestAll)
{
    const uint32_t size = 10000;

    Action expectedAction("Browse", g_controlUrl, ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "20000");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");
    
    InSequence seq;
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem"))));

    uint64_t startTime = timeops::getTimeInMilliSeconds();
    log::info("Start browse performance test");
    contentDirectory->browseDirectChildren(ContentDirectory::Client::All, "ObjectId", "filter", 0, size*2, "sort");
    log::info("Performance test finished: took {}ms", (timeops::getTimeInMilliSeconds() - startTime) / 1000.f);
}

TEST_F(ContentDirectoryTest, DISABLED_performanceTestContainersOnly)
{
    const uint32_t size = 10000;
    
    Action expectedAction("Browse", g_controlUrl, ServiceType::ContentDirectory);
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "10000");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");
    
    InSequence seq;
    EXPECT_CALL(client, sendAction(expectedAction))
        .WillOnce(Return(generateBrowseResponse(generateContainers(size, "object.container"), {})));
    
    uint64_t startTime = timeops::getTimeInMilliSeconds();
    log::info("Start browse performance test containers only");
    contentDirectory->browseDirectChildren(ContentDirectory::Client::ContainersOnly, "ObjectId", "filter", 0, size, "sort");
    log::info("Performance test finished: took {}ms", (timeops::getTimeInMilliSeconds() - startTime) / 1000.f);
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
