#include <gtest/gtest.h>
#include <gmock/gmock.h>

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

static const std::string s_hostedFile = "Very small file";

struct ResponseMock
{
    MOCK_METHOD2(onResponse, void(int32_t, size_t));
    MOCK_METHOD2(onResponse, void(int32_t, std::string));
    MOCK_METHOD2(onResponse, void(int32_t, std::vector<uint8_t>));
    MOCK_METHOD2(onResponse, void(int32_t, uint8_t*));
};

class HttpClientTest : public Test
{
public:
    HttpClientTest()
    : server(loop)
    , client(loop)
    {
        server.addFile("/test.txt", "plain/text", s_hostedFile);
        server.start(uv::Address::createIp4("127.0.0.1", 0));
    }

    template <typename Data>
    std::function<void(int32_t, Data)> handleResponse()
    {
        return [this] (int32_t status, Data data) {
            mock.onResponse(status, data);

            server.stop(nullptr);
        };
    }

    uv::Loop loop;
    http::Server server;
    http::Client client;
    ResponseMock mock;
};

TEST_F(HttpClientTest, DISABLED_ContentLengthNotProvided)
{
    bool gotCallback = false;
    client.getContentLength(server.getWebRootUrl() + "/test.txt", [&] (int32_t status, size_t /*size*/) {
        EXPECT_TRUE(status < 0);
        gotCallback = true;
    });

    loop.run(uv::RunMode::Default);
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, ContentLength)
{
    EXPECT_CALL(mock, onResponse(200, s_hostedFile.size()));
    auto url = server.getWebRootUrl() + "/test.txt";
    client.getContentLength(url, handleResponse<size_t>());
    loop.run(uv::RunMode::Default);
}

TEST_F(HttpClientTest, GetAsString)
{
    EXPECT_CALL(mock, onResponse(200, s_hostedFile));
    client.get(server.getWebRootUrl() + "/test.txt", handleResponse<std::string>());
    loop.run(uv::RunMode::Default);
}

TEST_F(HttpClientTest, GetAsVector)
{
    EXPECT_CALL(mock, onResponse(200, Matcher<std::vector<uint8_t>>(_))).WillOnce(WithArg<1>(Invoke([] (const std::vector<uint8_t>& data) {
        EXPECT_TRUE(std::equal(data.begin(), data.end(), s_hostedFile.begin(), s_hostedFile.end()));
    })));

    client.get(server.getWebRootUrl() + "/test.txt", handleResponse<std::vector<uint8_t>>());
    loop.run(uv::RunMode::Default);
}

TEST_F(HttpClientTest, GetAsArray)
{
    std::array<uint8_t, 15> array;
    array.fill(0);

    EXPECT_CALL(mock, onResponse(200, array.data()));
    client.get(server.getWebRootUrl() + "/test.txt", array.data(), handleResponse<uint8_t*>());
    loop.run(uv::RunMode::Default);

    EXPECT_TRUE(std::equal(array.begin(), array.end(), s_hostedFile.begin(), s_hostedFile.end()));
}

TEST_F(HttpClientTest, GetInvalidUrlAsString)
{
    EXPECT_CALL(mock, onResponse(404, ""));
    client.get(server.getWebRootUrl() + "/bad.txt", handleResponse<std::string>());
    loop.run(uv::RunMode::Default);
}

TEST_F(HttpClientTest, CouldNotConnect)
{
    EXPECT_CALL(mock, onResponse(-7, Matcher<size_t>(_)));
    client.setTimeout(5ms);
    client.getContentLength("http://127.0.0.1:81/index.html", handleResponse<size_t>());

    loop.run(uv::RunMode::Default);
}

}
}
