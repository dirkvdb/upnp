#include <catch.hpp>

#include "utils/log.h"
#include "upnp/upnp.http.client.h"
#include "upnp/upnp.http.server.h"

namespace upnp
{
namespace test
{

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

    // SECTION("Content length")
    // {
    //     client.getContentLength("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291468.mp3", [&] (int32_t status, size_t size) {
    //         INFO("GET Failed: " << http::Client::errorToString(status));
    //         CHECK(status == 0);
    //         CHECK(size > 0);
    //         gotCallback = true;
    //     });
    // }

    SECTION("Get as string")
    {
        client.get("http://www.google.be", [&] (int32_t status, std::string contents) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 0);
            CHECK_FALSE(contents.empty());
            gotCallback = true;
        });
    }

    SECTION("Get as vector")
    {
        client.get("http://www.google.be", [&] (int32_t status, std::vector<uint8_t> data) {
            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 0);
            CHECK_FALSE(data.empty());
            gotCallback = true;
        });
    }

    SECTION("Get as array")
    {
        auto data = std::make_unique<std::vector<uint8_t>>(1024 * 128, 0);
        uint8_t* originalDataPtr = data->data();
        client.get("http://www.google.be", originalDataPtr, [&] (int32_t status, uint8_t* dataPtr) {
            std::cout << status << std::endl << std::flush;

            INFO("GET Failed: " << http::Client::errorToString(status));
            CHECK(status == 0);
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
            CHECK(strcmp(http::Client::errorToString(status), "Timeout was reached") == 0);
            gotCallback = true;
        });
    }

    loop.run(uv::RunMode::Default);
    CHECK(gotCallback);
}

TEST_CASE("HTTP Client Server", "[HTTP]")
{
    uv::Loop        loop;
    http::Client    client(loop);
    http::Server    server(loop, 8080);
    bool gotCallback = false;

    client.get("http://localhost:8080/test.txt", [&] (int32_t status, std::string contents) {
        INFO("GET Failed: " << http::Client::errorToString(status));
        CHECK(status == 0);

        CHECK_FALSE(contents.empty());
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    CHECK(gotCallback);
}

}
}
