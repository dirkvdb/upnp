#include <catch.hpp>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp.http.parser.h"

namespace upnp
{
namespace test
{

using namespace utils;
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

TEST_CASE("HTTP Parse response", "[http]")
{
    http::Parser parser(http::Type::Response);
    bool completed = false;
    
    parser.setCompletedCallback([&] () {
        completed = true;
    });
    
    SECTION("One big chunk")
    {
        parser.parse(messageHeaders + messageBody);
    }
    
    SECTION("Body in second chunk")
    {
        parser.parse(messageHeaders);
        CHECK_FALSE(completed);
        parser.parse(messageBody);
    }
    
    SECTION("Completely chunked")
    {
        parser.parse(messageHeaders.substr(0, 20));
        CHECK_FALSE(completed);
        parser.parse(messageHeaders.substr(20, 8));
        CHECK_FALSE(completed);
        parser.parse(messageHeaders.substr(28, 15));
        CHECK_FALSE(completed);
        parser.parse(messageHeaders.substr(43));
        CHECK_FALSE(completed);
        parser.parse(messageBody.substr(0, 10));
        CHECK_FALSE(completed);
        parser.parse(messageBody.substr(10));
    }
    
    CHECK(completed);
    CHECK(parser.getStatus() == 200);
    CHECK(parser.headerValue("SERVER") == "Darwin/15.4.0, UPnP/1.0");
    CHECK(parser.headerValue("CONTENT-LENGTH") == "41");
    CHECK(parser.headerValue("Content-Length") == "41");
    CHECK(parser.headerValue("CONTENT-TYPE") == "text/html");
    CHECK(parser.headerValue("CONNECTION") == "closed");
    CHECK(parser.headerValue("EXT").empty());
    CHECK(parser.stealBody() == messageBody);
}

TEST_CASE("HTTP Parse double message", "[http]")
{
    http::Parser parser(http::Type::Response);
    uint32_t cbCount = 0;
    
    parser.setCompletedCallback([&] () {
        ++cbCount;
        CHECK(parser.headerValue("Content-Length") == ((cbCount == 1) ? "41" : "42"));
    });
    
    auto messageHeaders2 = messageHeaders;
    utils::stringops::replace(messageHeaders2, "41", "42");
    
    auto doubleMessage = messageHeaders + messageBody + messageHeaders2 + (messageBody + '!');
    CHECK(parser.parse(doubleMessage) == doubleMessage.size());
    CHECK(cbCount == 2);
}

}
}
