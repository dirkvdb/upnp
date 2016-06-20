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

using namespace asio;
using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string s_hostedFile = "Very small file";

struct ResponseMock
{
    MOCK_METHOD2(onResponse, void(http::errc::HttpError, size_t));
    MOCK_METHOD2(onResponse, void(http::errc::HttpError, std::string));
    MOCK_METHOD2(onResponse, void(http::errc::HttpError, std::vector<uint8_t>));
    MOCK_METHOD2(onResponse, void(http::errc::HttpError, uint8_t*));
};

class HttpClientTest : public Test
{
public:
    HttpClientTest()
    : server(io)
    , client(io)
    {
        server.addFile("/test.txt", "plain/text", s_hostedFile);
        server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 0));
    }

    template <typename Data>
    std::function<void(const std::error_code& error, Data)> handleResponse()
    {
        return [this] (const std::error_code& error, Data data) {
            mock.onResponse(http::errc::HttpError(error.value()), data);
            server.stop();
        };
    }

    io_service io;
    http::Server server;
    http::Client client;
    ResponseMock mock;
    std::thread ioThread;
};

TEST_F(HttpClientTest, DISABLED_ContentLengthNotProvided)
{
    bool gotCallback = false;
    client.getContentLength(server.getWebRootUrl() + "/test.txt", [&] (const std::error_code& error, size_t /*size*/) {
        EXPECT_TRUE(error.value() < 0);
        gotCallback = true;
    });

    io.run();
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, ContentLength)
{
    EXPECT_CALL(mock, onResponse(http::errc::Ok, s_hostedFile.size()));
    auto url = server.getWebRootUrl() + "/test.txt";
    client.getContentLength(url, handleResponse<size_t>());
    io.run();
}

TEST_F(HttpClientTest, GetAsString)
{
    EXPECT_CALL(mock, onResponse(http::errc::Ok, s_hostedFile));
    client.get(server.getWebRootUrl() + "/test.txt", handleResponse<std::string>());
    io.run();
}

TEST_F(HttpClientTest, GetAsVector)
{
    EXPECT_CALL(mock, onResponse(http::errc::Ok, Matcher<std::vector<uint8_t>>(_))).WillOnce(WithArg<1>(Invoke([] (const std::vector<uint8_t>& data) {
        EXPECT_TRUE(std::equal(data.begin(), data.end(), s_hostedFile.begin(), s_hostedFile.end()));
    })));

    client.get(server.getWebRootUrl() + "/test.txt", handleResponse<std::vector<uint8_t>>());
    io.run();
}

TEST_F(HttpClientTest, GetAsArray)
{
    std::array<uint8_t, 15> array;
    array.fill(0);

    EXPECT_CALL(mock, onResponse(http::errc::Ok, array.data()));
    client.get(server.getWebRootUrl() + "/test.txt", array.data(), handleResponse<uint8_t*>());
    io.run();

    EXPECT_TRUE(std::equal(array.begin(), array.end(), s_hostedFile.begin(), s_hostedFile.end()));
}

TEST_F(HttpClientTest, GetInvalidUrlAsString)
{
    EXPECT_CALL(mock, onResponse(http::errc::NotFound, ""));
    client.get(server.getWebRootUrl() + "/bad.txt", handleResponse<std::string>());
    io.run();
}

TEST_F(HttpClientTest, CouldNotConnect)
{
    EXPECT_CALL(mock, onResponse(http::errc::InvalidResponse, Matcher<size_t>(_)));
    client.setTimeout(5ms);
    client.getContentLength("http://127.0.0.1:81/index.html", handleResponse<size_t>());

    io.run();
}

}
}
