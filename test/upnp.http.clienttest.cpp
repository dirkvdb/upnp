#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.functions.h"
#include "upnp.http.client.h"
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
    MOCK_METHOD3(onResponse, void(std::error_code, http::StatusCode, size_t));
    MOCK_METHOD3(onResponse, void(std::error_code, http::StatusCode, std::string));
    MOCK_METHOD3(onResponse, void(std::error_code, http::StatusCode, uint8_t*));
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
    std::function<void(const std::error_code& error, http::StatusCode, Data)> handleResponse()
    {
        return [this] (const std::error_code& error, http::StatusCode status, Data data) {
            server.stop();
            mock.onResponse(error, status, data);
        };
    }

    std::function<void(const std::error_code& error, http::Response)> handleHttpResponse()
    {
        return [this] (const std::error_code& error, const http::Response& res) {
            server.stop();
            mock.onResponse(error, res.status, res.body);
        };
    }

    template <typename Ret, typename Awaitable>
    Future<Ret> waitForIt(io_service& io, Awaitable&& awaitable)
    {
        auto res = co_await awaitable;
        io.stop();
        co_return res;
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
    http::getContentLength(io, server.getWebRootUrl() + "/test.txt", [&] (const std::error_code& error, http::StatusCode status, size_t /*size*/) {
        EXPECT_FALSE(error) << "System error: " << error.message();
        EXPECT_EQ(http::StatusCode::Ok, status) << "HTTP error: " << status_message(status);
        gotCallback = true;
    });

    io.run();
    EXPECT_TRUE(gotCallback);
}

TEST_F(HttpClientTest, ContentLength)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::Ok, s_hostedFile.size()));
    http::getContentLength(io, server.getWebRootUrl() + "/test.txt", handleResponse<size_t>());
    io.run();
}

TEST_F(HttpClientTest, ContentLengthCoro)
{
    auto task = http::getContentLength(io, server.getWebRootUrl() + "/test.txt");

    Future<std::tuple<http::StatusCode, size_t>> res;
    io.post([&] () {
        res = waitForIt<std::tuple<http::StatusCode, size_t>>(io, task);
    });

    // io.post([&] () {
    //     auto fut = http::getContentLength(io, server.getWebRootUrl() + "/test.txt");
    //     std::thread t([this, fut = std::move(fut)] () mutable {
    //         auto res = sync_await(fut);
    //         EXPECT_EQ(http::StatusCode::Ok, std::get<0>(res));
    //         EXPECT_EQ(s_hostedFile.size(), std::get<1>(res));
    //     });

    //     t.join();
    // });

    io.run();

    auto [status, size] = res.get();
    EXPECT_EQ(http::StatusCode::Ok, status);
    EXPECT_EQ(s_hostedFile.size(), size);
}

TEST_F(HttpClientTest, GetAsString)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::Ok, s_hostedFile));
    http::get(io, server.getWebRootUrl() + "/test.txt", handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetAsArray)
{
    std::array<uint8_t, 15> array;
    array.fill(0);

    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::Ok, array.data()));
    http::get(io, server.getWebRootUrl() + "/test.txt", array.data(), handleResponse<uint8_t*>());
    io.run();

    EXPECT_TRUE(std::equal(array.begin(), array.end(), s_hostedFile.begin(), s_hostedFile.end()));
}

TEST_F(HttpClientTest, GetInvalidUrlAsString)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::NotFound, ""));
    http::get(io, server.getWebRootUrl() + "/bad.txt", handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetRange)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::PartialContent, "small fil"));
    http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 9, handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetRangeTillEnd)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::PartialContent, "small file"));
    http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 0, handleHttpResponse());
    io.run();
}


TEST_F(HttpClientTest, CouldNotConnect)
{
    EXPECT_CALL(mock, onResponse(std::make_error_code(http::error::NetworkError), _, Matcher<size_t>(_)));
    http::getContentLength(io, "http://127.0.0.1:81/index.html", handleResponse<size_t>());
    io.run();
}

}
}
