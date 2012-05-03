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
#include <gmock/gmock.h>
#include <iostream>


using namespace utils;
using namespace testing;

#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpmediaserver.h"
#include "upnp/upnpitem.h"

#include "testutils.h"
#include "testenvironment.h"

extern upnp::test::TestEnvironment* g_Env;

namespace upnp
{
namespace test
{

class SubscriberMock : public ISubscriber<Item>
{
public:
    MOCK_METHOD2(onItem, void(const Item&, void*));
    MOCK_METHOD1(finalItemReceived, void(void*));
    MOCK_METHOD1(onError, void(const std::string&));
};

class MediaServerTest : public Test
{
public:
    MediaServerTest()
    : m_Server(g_Env->getClient())
    {
    }

protected:
    virtual void SetUp()
    {
        m_Device = g_Env->getServer();
        ASSERT_TRUE(m_Device.get() != nullptr);

        m_Server.setDevice(m_Device);
    }

    virtual void TearDown()
    {
    }

    MediaServer                 m_Server;
    std::shared_ptr<Device>     m_Device;
};

TEST_F(MediaServerTest, DiscoveredServices)
{
    EXPECT_FALSE(m_Device->m_Services.end() == m_Device->m_Services.find(upnp::Service::ContentDirectory));
    EXPECT_FALSE(m_Device->m_Services.end() == m_Device->m_Services.find(upnp::Service::ConnectionManager));
    EXPECT_TRUE(m_Device->m_Services.end() == m_Device->m_Services.find(upnp::Service::AVTransport));
}

TEST_F(MediaServerTest, SearchCapabilities)
{
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Creator));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Date));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Title));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Album));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Actor));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Artist));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Class));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::Genre));
    EXPECT_TRUE(m_Server.canSearchForProperty(Property::RefId));
    EXPECT_FALSE(m_Server.canSearchForProperty(Property::All));
}

TEST_F(MediaServerTest, SortCapabilities)
{
    EXPECT_TRUE(m_Server.canSortOnProperty(Property::Date));
    EXPECT_TRUE(m_Server.canSortOnProperty(Property::Album));
    EXPECT_TRUE(m_Server.canSortOnProperty(Property::Title));
    EXPECT_TRUE(m_Server.canSortOnProperty(Property::Class));
    EXPECT_TRUE(m_Server.canSortOnProperty(Property::TrackNumber));
    EXPECT_FALSE(m_Server.canSortOnProperty(Property::All));
}

TEST_F(MediaServerTest, BrowseRootContainer)
{
    SubscriberMock subscriber;

    InSequence seq;
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Browse Folders"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Music"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Pictures"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Video"), nullptr));
    EXPECT_CALL(subscriber, finalItemReceived(nullptr));

    Item root(MediaServer::rootId);
    m_Server.getAllInContainer(root, subscriber);
}

TEST_F(MediaServerTest, BrowseRootContainerSorted)
{
    SubscriberMock subscriber;

    InSequence seq;
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Video"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Pictures"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Music"), nullptr));
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "Browse Folders"), nullptr));    
    
    EXPECT_CALL(subscriber, finalItemReceived(nullptr));

    Item root(MediaServer::rootId);
    m_Server.getAllInContainer(root, subscriber, Property::Title, MediaServer::SortMode::Descending);
}

TEST_F(MediaServerTest, SearchRootContainer)
{
    std::map<Property, std::string> criteria { {Property::Title, "Video"}, {Property::Class, "object.container"} };

    SubscriberMock subscriber;

    InSequence seq;
    EXPECT_CALL(subscriber, onItem(testing::Property(&Item::getTitle, "All Video"), nullptr));
    EXPECT_CALL(subscriber, finalItemReceived(nullptr));

    Item root(MediaServer::rootId);
    m_Server.search(root, criteria, subscriber);
}

TEST_F(MediaServerTest, SupportedActions)
{
    EXPECT_TRUE(m_Server.connectionManager().supportsAction(ConnectionManager::Action::GetProtocolInfo));
    EXPECT_TRUE(m_Server.connectionManager().supportsAction(ConnectionManager::Action::GetCurrentConnectionIDs));
    EXPECT_TRUE(m_Server.connectionManager().supportsAction(ConnectionManager::Action::GetCurrentConnectionInfo));
    EXPECT_FALSE(m_Server.connectionManager().supportsAction(ConnectionManager::Action::PrepareForConnection));
    EXPECT_FALSE(m_Server.connectionManager().supportsAction(ConnectionManager::Action::ConnectionComplete));
}

}
}
