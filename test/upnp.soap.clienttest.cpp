#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.functions.h"
#include "upnp.soap.client.h"
#include "upnp.http.client.h"
#include "upnp/upnp.http.server.h"
#include "upnp.testutils.h"

namespace upnp
{
namespace test
{

using namespace asio;
using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace
{

struct RequestMock
{
    MOCK_METHOD1(onRequest, std::string(const http::Request&));
};

struct ResponseMock
{
    MOCK_METHOD2(onResponse, void(std::error_code, soap::ActionResult));
};

}

class SoapClientTest : public Test
{
public:
    SoapClientTest()
    : server(io)
    , client(io)
    {
        server.setRequestHandler(http::Method::Post, [this] (const http::Request& req) {
            return reqMock.onRequest(req);
        });

        server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 0));
    }

    std::function<void(const std::error_code& error, soap::ActionResult data)> handleResponse()
    {
        return [this] (const std::error_code& error, soap::ActionResult res) {
            server.stop();
            resMock.onResponse(error, res);
        };
    }

    io_service io;
    http::Server server;
    soap::Client client;
    ResponseMock resMock;
    RequestMock reqMock;
};

TEST_F(SoapClientTest, SoapAction)
{
    auto envelope = "data"s;

    auto body = "<html><body><h1>200 OK</h1></body></html>"s;
    auto response =
        "HTTP/1.1 200 OK\r\n"
        "CONTENT-LENGTH: {}\r\n"
        "\r\n"
        "{}";

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (const http::Request& request) {
        EXPECT_EQ("/soap", request.url());
        EXPECT_EQ("\"ServiceName#ActionName\"", request.field("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", request.field("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), request.field("Content-Length"));
        return fmt::format(response, body.size(), body);
    }));

    EXPECT_CALL(resMock, onResponse(std::error_code(), soap::ActionResult(http::StatusCode::Ok, body))).WillOnce(WithArgs<1>(Invoke([] (auto& res) {
        EXPECT_FALSE(res.isFaulty()) << "Unexpected soap fault assigned";
    })));

    client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope, handleResponse());
    io.run();
}

TEST_F(SoapClientTest, SoapActionCoro)
{
    auto envelope = "data"s;

    auto body = "<html><body><h1>200 OK</h1></body></html>"s;
    auto response =
        "HTTP/1.1 200 OK\r\n"
        "CONTENT-LENGTH: {}\r\n"
        "\r\n"
        "{}";

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (const http::Request& request) {
        EXPECT_EQ("/soap", request.url());
        EXPECT_EQ("\"ServiceName#ActionName\"", request.field("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", request.field("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), request.field("Content-Length"));
        return fmt::format(response, body.size(), body);
    }));

    auto actionResult = runCoroTask(io, client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope));
    EXPECT_EQ(http::StatusCode::Ok, actionResult.httpStatus);
    EXPECT_EQ(body, actionResult.response);
    EXPECT_FALSE(actionResult.isFaulty());
}

TEST_F(SoapClientTest, SoapActionWithFault)
{
    auto envelope = "data"s;

    auto body =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "  <s:Body>"
        "      <s:Fault>"
        "      <faultcode>s:Client</faultcode>"
        "      <faultstring>UPnPError</faultstring>"
        "      <detail>"
        "        <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
        "          <errorCode>718</errorCode>"
        "          <errorDescription>ConflictInMappingEntry</errorDescription>"
        "        </UPnPError>"
        "      </detail>"
        "    </s:Fault>"
        "  </s:Body>"
        "</s:Envelope>"s;

    auto errorResponse =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
        "CONTENT-LENGTH: {}\r\n"
        "\r\n"
        "{}";

    auto response = fmt::format(errorResponse, body.size(), body);

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (const http::Request& request) {
        EXPECT_EQ("/soap", request.url());
        EXPECT_EQ("\"ServiceName#ActionName\"", request.field("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", request.field("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), request.field("Content-Length"));
        return response;
    }));

    EXPECT_CALL(resMock, onResponse(std::error_code(), soap::ActionResult(http::StatusCode::InternalServerError, body))).WillOnce(WithArgs<1>(Invoke([] (auto& response) {
        ASSERT_TRUE(response.isFaulty()) << "No soap fault assigned";
        auto fault = response.getFault();
        EXPECT_EQ(718u, fault.errorCode());
        EXPECT_EQ("ConflictInMappingEntry"s, fault.errorDescription());
    })));

    client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope, handleResponse());
    io.run();
}

TEST_F(SoapClientTest, SoapActionWithFaultCoro)
{
    auto envelope = "data"s;

    auto body =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "  <s:Body>"
        "      <s:Fault>"
        "      <faultcode>s:Client</faultcode>"
        "      <faultstring>UPnPError</faultstring>"
        "      <detail>"
        "        <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
        "          <errorCode>718</errorCode>"
        "          <errorDescription>ConflictInMappingEntry</errorDescription>"
        "        </UPnPError>"
        "      </detail>"
        "    </s:Fault>"
        "  </s:Body>"
        "</s:Envelope>"s;

    auto errorResponse =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
        "CONTENT-LENGTH: {}\r\n"
        "\r\n"
        "{}";

    auto response = fmt::format(errorResponse, body.size(), body);

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (const http::Request& request) {
        EXPECT_EQ("/soap", request.url());
        EXPECT_EQ("\"ServiceName#ActionName\"", request.field("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", request.field("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), request.field("Content-Length"));
        return response;
    }));

    auto actionResult = runCoroTask(io, client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope));
    EXPECT_EQ(http::StatusCode::InternalServerError, actionResult.httpStatus);
    EXPECT_EQ(body, actionResult.response);
    EXPECT_TRUE(actionResult.isFaulty());
    auto fault = actionResult.getFault();
    EXPECT_EQ(718u, fault.errorCode());
    EXPECT_EQ("ConflictInMappingEntry"s, fault.errorDescription());
}

}
}
