#include <catch.hpp>

#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.client.h"
#include "upnp/upnp.http.server.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("HTTP Client", "[HTTP]")
{
    uv::Loop        loop;
    http::Client    client(loop);
    bool gotCallback = false;

    SECTION("ContentLength not provided")
    {
        client.getContentLength("http://www.google.be", [&] (int32_t status, size_t /*size*/) {
            CHECK(status < 0);
            gotCallback = true;
        });
    }

    SECTION("Content length")
    {
        client.getContentLength("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291468.mp3", [&] (int32_t status, size_t size) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 0);
            CHECK(size > 0);
            gotCallback = true;
        });
    }

    SECTION("Get as string")
    {
        client.get("http://www.google.be", [&] (int32_t status, std::string contents) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 200);
            CHECK_FALSE(contents.empty());
            gotCallback = true;
        });
    }

    SECTION("Get as vector")
    {
        client.get("http://www.google.be", [&] (int32_t status, std::vector<uint8_t> data) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 200);
            CHECK_FALSE(data.empty());
            gotCallback = true;
        });
    }

    SECTION("Get as array")
    {
        std::array<uint8_t, 1024 * 128> array;
        array.fill(0);
        uint8_t* originalDataPtr = array.data();

        client.get("http://www.google.be", originalDataPtr, [&, data = std::move(array)] (int32_t status, uint8_t* dataPtr) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 200);
            CHECK(dataPtr[0] != 0);
            CHECK(originalDataPtr == dataPtr);
            gotCallback = true;
        });
    }

    SECTION("Get invalid url as string")
    {
        client.get("http://www.googlegooglegooglegooglegooglegoogle.be", [&] (int32_t status, std::string contents) {
            CHECK(status < 0);
            CHECK(contents.empty());
            gotCallback = true;
        });
    }

    SECTION("Get server url")
    {
        client.get("http://localhost:8080/test.txt", [&] (int32_t status, std::string contents) {
            CHECK(status < 0);
            CHECK(contents.empty());
            gotCallback = true;
        });
    }

    SECTION("Invalid url")
    {
        client.setTimeout(5ms);
        client.getContentLength("http://192.168.55.245/index.html", [&] (int32_t status, size_t /*size*/) {
            CHECK(status < 0);
            CHECK(http::Client::errorToString(status) == "Timeout was reached");
            gotCallback = true;
        });
    }

    loop.run(uv::RunMode::Default);
    uv::stopLoopAndCloseRequests(loop);
    CHECK(gotCallback);
}

TEST_CASE("HTTP Client Server", "[HTTP]")
{
    uv::Loop        loop;
    http::Client    client(loop);
    http::Server    server(loop, 8080);
    bool gotCallback = false;

    auto servedFile = "This is my amazing file"s;
    server.addFile("/test.txt", "text/plain", servedFile);

    client.get("http://localhost:8080/test.txt", [&] (int32_t status, std::string contents) {
        INFO("GET Failed: " << http::Client::errorToString(status));
        CHECK(status == 200);

        CHECK(contents == servedFile);
        gotCallback = true;

        stopLoopAndCloseRequests(loop);
    });

    loop.run(uv::RunMode::Default);
    CHECK(gotCallback);
}

TEST_CASE("HTTP Client Soap action invalid url", "[SOAP]")
{
    uv::Loop        loop;
    http::Client    client(loop);
    bool gotCallback = false;

    Action action("SetVolume", "http://192.168.1.13:9000/dev0/srv0/control", ServiceType::ContentDirectory);

    client.setTimeout(1s);
    client.soapAction(action.getUrl(),
                      action.getName(),
                      action.getServiceTypeUrn(),
                      action.toString(),
                      [&] (int32_t status, std::string /*res*/) {
        INFO("Unexpected status code: " << http::Client::errorToString(status));
        CHECK(status == 500);

        //log::info(res);
        gotCallback = true;
        stopLoopAndCloseRequests(loop);
    });

    loop.run(uv::RunMode::Default);
    CHECK(gotCallback);
}

}
}
