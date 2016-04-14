#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.client.h"
#include "upnp/upnp.http.server.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

class HttpClientTest : public Test
{
public:
    HttpClientTest()
    : client(loop)
    {
    }

    uv::Loop loop;
    http::Client client;
};

TEST_F(HttpClientTest, ContentLengthNotProvided)
{
    bool gotCallback = false;
    client.getContentLength("http://www.google.be", [&] (int32_t status, size_t /*size*/) {
        EXPECT_TRUE(status < 0);
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, DISABLED_ContentLength)
{
    bool gotCallback = false;
    client.getContentLength("http://192.168.1.13:9000/disk/DLNA-PNMP3-OP11-FLAGS01700000/O0$1$8I2291468.mp3", [&] (int32_t status, size_t size) {
        EXPECT_EQ(9, status) << "GET Failed: " << http::Client::errorToString(status);
        EXPECT_TRUE(size > 0);
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, GetAsString)
{
    bool gotCallback = false;
    client.get("http://www.google.be", [&] (int32_t status, std::string contents) {
        EXPECT_EQ(200, status) << "GET Failed: " << http::Client::errorToString(status);
        EXPECT_FALSE(contents.empty());
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, GetAsVector)
{
    bool gotCallback = false;
    client.get("http://www.google.be", [&] (int32_t status, std::vector<uint8_t> data) {
        EXPECT_EQ(200, status) << "GET Failed: " << http::Client::errorToString(status);
        EXPECT_FALSE(data.empty());
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, GetAsArray)
{
    bool gotCallback = false;
    std::array<uint8_t, 1024 * 128> array;
    array.fill(0);
    uint8_t* originalDataPtr = array.data();

    client.get("http://www.google.be", originalDataPtr, [&, data = std::move(array)] (int32_t status, uint8_t* dataPtr) {
        EXPECT_EQ(200, status) << "GET Failed: " << http::Client::errorToString(status);
        EXPECT_NE(0, dataPtr[0]);
        EXPECT_EQ(originalDataPtr, dataPtr);
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, GetInvalidUrlAsString)
{
    bool gotCallback = false;
    client.get("http://www.googlegooglegooglegooglegooglegoogle.be", [&] (int32_t status, std::string contents) {
        EXPECT_TRUE(status < 0);
        EXPECT_TRUE(contents.empty());
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, GetServerUrl)
{
    bool gotCallback = false;
    client.get("http://localhost:8080/test.txt", [&] (int32_t status, std::string contents) {
        EXPECT_TRUE(status < 0);
        EXPECT_TRUE(contents.empty());
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, InvalidUrl)
{
    bool gotCallback = false;
    client.setTimeout(5ms);
    client.getContentLength("http://192.168.55.245/index.html", [&] (int32_t status, size_t /*size*/) {
        EXPECT_TRUE(status < 0);
        EXPECT_EQ("Timeout was reached"s, http::Client::errorToString(status));
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, ClientServer)
{
    uv::Loop        loop;
    http::Client    client(loop);
    http::Server    server(loop, 8080);
    bool gotCallback = false;

    auto servedFile = "This is my amazing file"s;
    server.addFile("/test.txt", "text/plain", servedFile);

    client.get("http://localhost:8080/test.txt", [&] (int32_t status, std::string contents) {
        EXPECT_EQ(200, status) << "GET Failed: " << http::Client::errorToString(status);
        EXPECT_EQ(servedFile, contents);
        gotCallback = true;

        loop.stop();
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, SoapActionInvalidUrl)
{
    bool gotCallback = false;

    Action action("SetVolume", "http://192.168.1.13:9000/dev0/srv0/control", ServiceType::ContentDirectory);

    client.setTimeout(1s);
    client.soapAction(action.getUrl(),
                      action.getName(),
                      action.getServiceTypeUrn(),
                      action.toString(),
                      [&] (int32_t status, std::string /*res*/) {
        EXPECT_EQ(500, status) << "Unexpected status code: " << http::Client::errorToString(status);

        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

}
}
