#include "gtest/gtest.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp/upnp.http.parser.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

auto messageHeaders =
            "HTTP/1.1 200 OK\r\n"
            "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
            "EXT:\r\n"
            "CONTENT-LENGTH: 41\r\n"
            "CONTENT-TYPE: text/html\r\n"
            "CONNECTION: closed\r\n"
            "\r\n"s;

auto messageBody = "<html><body><h1>200 OK</h1></body></html>"s;

class HttpParserTest : public Test
{
public:
    HttpParserTest()
    : parser(http::Type::Response)
    {
        parser.setCompletedCallback([&] () {
            completed = true;
        });
    }

    void ExpectReferenceHeaderParsed()
    {
        EXPECT_TRUE(completed);
        EXPECT_EQ(200u, parser.getStatus());
        EXPECT_EQ("Darwin/15.4.0, UPnP/1.0"s, parser.headerValue("SERVER"));
        EXPECT_EQ("41"s, parser.headerValue("CONTENT-LENGTH"));
        EXPECT_EQ("41"s, parser.headerValue("Content-Length"));
        EXPECT_EQ("text/html"s, parser.headerValue("CONTENT-TYPE"));
        EXPECT_EQ("closed"s, parser.headerValue("CONNECTION"));
        EXPECT_TRUE(parser.headerValue("EXT").empty());
        EXPECT_EQ(messageBody, parser.stealBody());
    }

    http::Parser parser;
    bool completed = false;
};

TEST_F(HttpParserTest, OneBigChunk)
{
    parser.parse(messageHeaders + messageBody);
    ExpectReferenceHeaderParsed();
}

TEST_F(HttpParserTest, BodyInSecondChunk)
{
    parser.parse(messageHeaders);
    EXPECT_FALSE(completed);
    parser.parse(messageBody);
    ExpectReferenceHeaderParsed();
}

TEST_F(HttpParserTest, CompletelyChunked)
{
    parser.parse(messageHeaders.substr(0, 20));
    EXPECT_FALSE(completed);
    parser.parse(messageHeaders.substr(20, 8));
    EXPECT_FALSE(completed);
    parser.parse(messageHeaders.substr(28, 15));
    EXPECT_FALSE(completed);
    parser.parse(messageHeaders.substr(43));
    EXPECT_FALSE(completed);
    parser.parse(messageBody.substr(0, 10));
    EXPECT_FALSE(completed);
    parser.parse(messageBody.substr(10));
    ExpectReferenceHeaderParsed();
}

TEST_F(HttpParserTest, ParseDoubleMessage)
{
    uint32_t cbCount = 0;
    parser.setCompletedCallback([&] () {
        ++cbCount;
        EXPECT_EQ(((cbCount == 1) ? "41" : "42"), parser.headerValue("Content-Length"));
    });

    auto messageHeaders2 = messageHeaders;
    utils::str::replace(messageHeaders2, "41", "42");

    auto doubleMessage = messageHeaders + messageBody + messageHeaders2 + (messageBody + '!');
    EXPECT_EQ(doubleMessage.size(), parser.parse(doubleMessage));
    EXPECT_EQ(2u, cbCount);
}

}
}
