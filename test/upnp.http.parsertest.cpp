#include <catch.hpp>

#include "utils/log.h"
#include "upnp.http.parser.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("HTTP Parse response", "[http]")
{
    http::Parser parser(http::Type::Response);
    bool completed = false;
    
    parser.setCompletedCallback([&] () {
        completed = true;
    });
    
    auto messageHeaders =
            "HTTP/1.1 200 OK\r\n"
            "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
            "CONTENT-LENGTH: 41\r\n"
            "CONTENT-TYPE: text/html\r\n"
            "CONNECTION: closed\r\n"
            "\r\n"s;
            "<html><body><h1>200 OK</h1></body></html>"s;
        
    auto messageBody = "<html><body><h1>200 OK</h1></body></html>"s;

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
    
    CHECK(completed);
    CHECK(parser.getStatus() == 200);
    CHECK(parser.headerValue("SERVER") == "Darwin/15.4.0, UPnP/1.0");
    CHECK(parser.headerValue("CONTENT-LENGTH") == "41");
    CHECK(parser.headerValue("Content-Length") == "41");
    CHECK(parser.headerValue("CONTENT-TYPE") == "text/html");
    CHECK(parser.headerValue("CONNECTION") == "closed");
    CHECK(parser.stealBody() == messageBody);
}

}
}
