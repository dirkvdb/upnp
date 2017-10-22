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

#include "upnp.clienttestbase.h"
#include "upnp/upnp.contentdirectory.client.h"

using namespace utils;
using namespace testing;
using namespace std::placeholders;

namespace upnp
{
namespace test
{

static const std::string s_eventNameSpaceId = "CDS";
using ActionResult = ContentDirectory::ActionResult;

class ItemSubscriber
{
public:
    MOCK_METHOD1(onItem, void(const std::shared_ptr<Item>&));
};

struct ContentDirectoryStatusCallbackMock
{
    MOCK_METHOD1(onStatus, void(Status));
    MOCK_METHOD2(onStatus, void(Status, Item));
    MOCK_METHOD2(onStatus, void(Status, ActionResult));
    MOCK_METHOD2(onStatus, void(Status, std::vector<Property>));
};

class ContentDirectoryClientTest : public ServiceClientTestBase<ContentDirectory::Client, ContentDirectoryStatusCallbackMock, ContentDirectory::Variable>
{
public:
    ContentDirectoryClientTest() : ServiceClientTestBase(testxmls::contentDirectoryServiceDescription)
    {
    }

    void expectItem(uint32_t index, const Item& item)
    {
        std::string indexStr = getIndexString(index);

        EXPECT_EQ("Id" + indexStr,          item.getObjectId());
        EXPECT_EQ("ParentId",               item.getParentId());
        EXPECT_EQ("Title" + indexStr,       item.getTitle());
        EXPECT_EQ(0U,                       item.getChildCount());

        EXPECT_EQ("Actor" + indexStr,       item.getMetaData(Property::Actor));
        EXPECT_EQ("Album" + indexStr,       item.getMetaData(Property::Album));
        EXPECT_EQ("AlbumArt" + indexStr,    item.getMetaData(Property::AlbumArt));
        EXPECT_EQ("Genre" + indexStr,       item.getMetaData(Property::Genre));
        EXPECT_EQ("Description" + indexStr, item.getMetaData(Property::Description));
        EXPECT_EQ("01/01/196" + indexStr,   item.getMetaData(Property::Date));
        EXPECT_EQ(indexStr,                 item.getMetaData(Property::TrackNumber));
    }

    void expectContainer(uint32_t index, const Item& item)
    {
        std::string indexStr = getIndexString(index);

        EXPECT_EQ("Id" + indexStr,          item.getObjectId());
        EXPECT_EQ("ParentId",               item.getParentId());
        EXPECT_EQ("Title" + indexStr,       item.getTitle());
        EXPECT_EQ(index,                    item.getChildCount());

        EXPECT_EQ("Artist" + indexStr,      item.getMetaData(Property::Artist));
        EXPECT_EQ("Creator" + indexStr,     item.getMetaData(Property::Creator));
        EXPECT_EQ("AlbumArt" + indexStr,    item.getMetaData(Property::AlbumArt));
        EXPECT_EQ("Genre" + indexStr,       item.getMetaData(Property::Genre));

        EXPECT_TRUE(item.getMetaData(Property::Actor).empty());
        EXPECT_TRUE(item.getMetaData(Property::Album).empty());
        EXPECT_TRUE(item.getMetaData(Property::Description).empty());
        EXPECT_TRUE(item.getMetaData(Property::Date).empty());
        EXPECT_TRUE(item.getMetaData(Property::TrackNumber).empty());
    }
};

TEST_F(ContentDirectoryClientTest, querySearchCapabilities)
{
    Action searchCaps("GetSearchCapabilities", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectAction(searchCaps, { { "SearchCaps", "upnp:artist,dc:title" } });

    std::vector<Property> props = { Property::Artist, Property::Title };
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<std::vector<Property>>(ContainerEq(props))));
    serviceInstance->querySearchCapabilities(checkStatusCallback<std::vector<Property>>());
}

TEST_F(ContentDirectoryClientTest, querySearchCapabilitiesCoro)
{
    Action searchCaps("GetSearchCapabilities", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    std::vector<std::pair<std::string, std::string>> responseVars = { {"SearchCaps", "upnp:artist,dc:title"} };
    expectActionCoroResponse(searchCaps, responseVars);

    std::vector<Property> props = { Property::Artist, Property::Title };
    EXPECT_THAT(serviceInstance->querySearchCapabilities().get(), ContainerEq(props));
}

TEST_F(ContentDirectoryClientTest, querySortCapabilities)
{
    Action sortCaps("GetSortCapabilities", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectAction(sortCaps, { { "SortCaps", "upnp:artist,dc:title,upnp:genre" } });

    std::vector<Property> props = {Property::Artist, Property::Title, Property::Genre};
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<std::vector<Property>>(ContainerEq(props))));
    serviceInstance->querySortCapabilities(checkStatusCallback<std::vector<Property>>());
}

TEST_F(ContentDirectoryClientTest, querySortCapabilitiesCoro)
{
    Action sortCaps("GetSortCapabilities", s_controlUrl, {ServiceType::ContentDirectory, 1});
    std::vector<std::pair<std::string, std::string>> responseVars = { {"SortCaps", "upnp:artist,dc:title,upnp:genre"} };
    expectActionCoroResponse(sortCaps, responseVars);

    std::vector<Property> props = {Property::Artist, Property::Title, Property::Genre};
    EXPECT_THAT(serviceInstance->querySortCapabilities().get(), ContainerEq(props));
}

TEST_F(ContentDirectoryClientTest, supportedActions)
{
    EXPECT_TRUE(serviceInstance->supportsAction(ContentDirectory::Action::GetSearchCapabilities));
    EXPECT_TRUE(serviceInstance->supportsAction(ContentDirectory::Action::GetSortCapabilities));
    EXPECT_TRUE(serviceInstance->supportsAction(ContentDirectory::Action::GetSystemUpdateID));
    EXPECT_TRUE(serviceInstance->supportsAction(ContentDirectory::Action::Browse));
    EXPECT_TRUE(serviceInstance->supportsAction(ContentDirectory::Action::Search));
}

TEST_F(ContentDirectoryClientTest, browseMetadataItem)
{
    Action expectedAction("Browse", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("BrowseFlag", "BrowseMetadata");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "0");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    InSequence seq;
    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse({}, generateItems(1, "object.item.audioItem")));
    }));

    Item item;
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<Item>(_))).WillOnce(SaveArg<1>(&item));
    serviceInstance->browseMetadata("ObjectId", "filter", checkStatusCallback<Item>());
    expectItem(0, item);
}

TEST_F(ContentDirectoryClientTest, browseMetadataContainer)
{
    Action expectedAction("Browse", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("BrowseFlag", "BrowseMetadata");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "0");
    expectedAction.addArgument("SortCriteria", "");
    expectedAction.addArgument("StartingIndex", "0");

    InSequence seq;
    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(1, "object.container"), {}));
    }));

    Item item;
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<Item>(_))).WillOnce(SaveArg<1>(&item));
    serviceInstance->browseMetadata("ObjectId", "filter", checkStatusCallback<Item>());
    expectContainer(0, item);
}

TEST_F(ContentDirectoryClientTest, browseDirectChildren)
{
    const uint32_t size = 10;

    Action expectedAction("Browse", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "100");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");

    InSequence seq;
    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem")));
    }));

    ActionResult result;
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<ActionResult>(_))).WillOnce(SaveArg<1>(&result));
    serviceInstance->browseDirectChildren(ContentDirectory::Client::All, "ObjectId", "filter", 0, 100, "sort", checkStatusCallback<ActionResult>());

    EXPECT_EQ(size * 2, result.totalMatches);
    EXPECT_EQ(size * 2, result.numberReturned);

    uint32_t containerCount = 0;
    uint32_t itemCount = 0;
    for (auto& item : result.result)
    {
        if (item.getClass() == Class::Container)
        {
            expectContainer(containerCount++, item);
        }
        else if (item.getClass() == Class::Audio)
        {
            expectItem(itemCount++, item);
        }
        else
        {
            FAIL() << "Unexpected class type encountered";
        }
    }
}

TEST_F(ContentDirectoryClientTest, search)
{
    const uint32_t size = 10;

    Action expectedAction("Search", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("Filter", "filt");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "100");
    expectedAction.addArgument("SearchCriteria", "crit");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");

    InSequence seq;
    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem")));
    }));

    ActionResult result;
    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<ActionResult>(_))).WillOnce(SaveArg<1>(&result));
    serviceInstance->search("ObjectId", "crit", "filt", 0, 100, "sort", checkStatusCallback<ActionResult>());

    EXPECT_EQ(size * 2, result.totalMatches);
    EXPECT_EQ(size * 2, result.numberReturned);

    uint32_t containerCount = 0;
    uint32_t itemCount = 0;
    for (auto& item : result.result)
    {
        if (item.getClass() == Class::Container)
        {
            expectContainer(containerCount++, item);
        }
        else if (item.getClass() == Class::Audio)
        {
            expectItem(itemCount++, item);
        }
        else
        {
            FAIL() << "Unexpected class type encountered";
        }
    }
}

TEST_F(ContentDirectoryClientTest, DISABLED_performanceTestAll)
{
    const uint32_t size = 10000;

    Action expectedAction("Browse", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "20000");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem")));
    }));

    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<ActionResult>(_)));

    uint64_t startTime = timeops::getTimeInMilliSeconds();
    log::info("Start browse performance test");
    serviceInstance->browseDirectChildren(ContentDirectory::Client::All, "ObjectId", "filter", 0, size*2, "sort", checkStatusCallback<ActionResult>());
    log::info("Performance test finished: took {}ms", (timeops::getTimeInMilliSeconds() - startTime) / 1000.f);
}

TEST_F(ContentDirectoryClientTest, DISABLED_performanceTestContainersOnly)
{
    const uint32_t size = 10000;

    Action expectedAction("Browse", s_controlUrl, { ServiceType::ContentDirectory, 1 });
    expectedAction.addArgument("BrowseFlag", "BrowseDirectChildren");
    expectedAction.addArgument("Filter", "filter");
    expectedAction.addArgument("ObjectID", "ObjectId");
    expectedAction.addArgument("RequestedCount", "10000");
    expectedAction.addArgument("SortCriteria", "sort");
    expectedAction.addArgument("StartingIndex", "0");

    EXPECT_CALL(client, sendAction(_, _)).WillOnce(Invoke([&] (auto& action, auto& cb) {
        EXPECT_EQ(expectedAction.toString(), action.toString());
        cb(Status(), generateBrowseResponse(generateContainers(size, "object.container"),
                                                generateItems(size, "object.item.audioItem")));
    }));

    EXPECT_CALL(statusMock, onStatus(Status(), Matcher<ActionResult>(_)));

    uint64_t startTime = timeops::getTimeInMilliSeconds();
    log::info("Start browse performance test containers only");
    serviceInstance->browseDirectChildren(ContentDirectory::Client::ContainersOnly, "ObjectId", "filter", 0, size, "sort", checkStatusCallback<ActionResult>());
    log::info("Performance test finished: took {}ms", (timeops::getTimeInMilliSeconds() - startTime) / 1000.f);
}

}
}
