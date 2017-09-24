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
    Future<Ret> waitForIt(Awaitable&& awaitable)
    {
        try
        {
            auto res = co_await awaitable;
            io.stop();
            co_return res;

        }
        catch (...)
        {
            io.stop();
            std::rethrow_exception(std::current_exception());
        }
    }

    template <typename TaskResult>
    auto runCoroTask(Future<TaskResult>&& task)
    {
        Future<TaskResult> res;
        io.post([&] () {
            res = waitForIt<TaskResult>(task);
        });

        io.run();
        return res.get();
    }

    io_service io;
    http::Server server;
    http::Client client;
    ResponseMock mock;
    std::thread ioThread;
};

TEST_F(HttpClientTest, ContentLength)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::Ok, s_hostedFile.size()));
    http::getContentLength(io, server.getWebRootUrl() + "/test.txt", handleResponse<size_t>());
    io.run();
}

TEST_F(HttpClientTest, ContentLengthCoro)
{
    auto [status, size] = runCoroTask(http::getContentLength(io, server.getWebRootUrl() + "/test.txt"));
    EXPECT_EQ(http::StatusCode::Ok, status);
    EXPECT_EQ(s_hostedFile.size(), size);
}

TEST_F(HttpClientTest, GetAsString)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::Ok, s_hostedFile));
    http::get(io, server.getWebRootUrl() + "/test.txt", handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetAsStringCoro)
{
    auto response = runCoroTask(http::get(io, server.getWebRootUrl() + "/test.txt"));
    EXPECT_EQ(http::StatusCode::Ok, response.status);
    EXPECT_EQ(s_hostedFile, response.body);
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

TEST_F(HttpClientTest, GetAsArrayCoro)
{
    std::array<uint8_t, 15> array = {};

    auto status = runCoroTask(http::get(io, server.getWebRootUrl() + "/test.txt", array.data()));
    EXPECT_EQ(http::StatusCode::Ok, status);
    EXPECT_TRUE(std::equal(array.begin(), array.end(), s_hostedFile.begin(), s_hostedFile.end()));
}

TEST_F(HttpClientTest, GetInvalidUrlAsString)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::NotFound, ""));
    http::get(io, server.getWebRootUrl() + "/bad.txt", handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetInvalidUrlAsStringCoro)
{
    auto response = runCoroTask(http::get(io, server.getWebRootUrl() + "/bad.txt"));
    EXPECT_EQ(http::StatusCode::NotFound, response.status);
    EXPECT_TRUE(response.body.empty());
}

TEST_F(HttpClientTest, GetRange)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::PartialContent, "small fil"));
    http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 9, handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetRangeCoro)
{
    auto response = runCoroTask(http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 9));
    EXPECT_EQ(http::StatusCode::PartialContent, response.status);
    EXPECT_EQ("small fil"s, response.body);
}

TEST_F(HttpClientTest, GetRangeTillEnd)
{
    EXPECT_CALL(mock, onResponse(std::error_code(), http::StatusCode::PartialContent, "small file"));
    http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 0, handleHttpResponse());
    io.run();
}

TEST_F(HttpClientTest, GetRangeTillEndCoro)
{
    auto response = runCoroTask(http::getRange(io, server.getWebRootUrl() + "/test.txt", 5, 0));
    EXPECT_EQ(http::StatusCode::PartialContent, response.status);
    EXPECT_EQ("small file"s, response.body);
}

TEST_F(HttpClientTest, CouldNotConnect)
{
    EXPECT_CALL(mock, onResponse(std::make_error_code(http::error::NetworkError), _, Matcher<size_t>(_)));
    http::getContentLength(io, "http://127.0.0.1:81/index.html", handleResponse<size_t>());
    io.run();
}

TEST_F(HttpClientTest, CouldNotConnectCoro)
{
    try
    {
        runCoroTask(http::getContentLength(io, "http://127.0.0.1:81/index.html"));
        FAIL() << "No exception thrown";
    }
    catch (const std::system_error& e)
    {
        EXPECT_EQ(std::make_error_code(http::error::NetworkError), e.code());
    }
    catch (...)
    {
        FAIL() << "Wrong exception type thrown";
    }
}

}
}
