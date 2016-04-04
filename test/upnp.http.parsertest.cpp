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

TEST_CASE("HTTP Parse response", "[HTTP]")
{
    http::Parser parser(http::Type::Response);
    parser.setHeaderCallback([] (const char *, size_t, const char *, size_t) {
        
    });

    SECTION("One big chunk")
    {
        auto message =
            "HTTP/1.1 200 OK\r\n"
            "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
            "CONNECTION: close\r\n"
            "CONTENT-LENGTH: 41\r\n"
            "CONTENT-TYPE: text/html\r\n"
            "\r\n"
            "<html><body><h1>200 OK</h1></body></html>"s;
    
        parser.parse(message.data(), message.size());
    }
}

}
}
