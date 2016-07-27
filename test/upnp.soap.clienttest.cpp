#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.functions.h"
#include "upnp/upnp.soap.client.h"
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

namespace
{

struct RequestMock
{
    MOCK_METHOD1(onRequest, std::string(http::Parser&));
};

struct ResponseMock
{
    MOCK_METHOD2(onResponse, void(std::error_code, std::string));
};

}

class SoapClientTest : public Test
{
public:
    SoapClientTest()
    : server(io)
    , client(io)
    {
        server.setRequestHandler(http::Method::Post, [this] (http::Parser& parser) {
            return reqMock.onRequest(parser);
        });

        server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 0));
    }

    std::function<void(const std::error_code& error, const std::string& data)> handleResponse()
    {
        return [this] (const std::error_code& error, const std::string& data) {
            server.stop();
            resMock.onResponse(error, data);
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

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (http::Parser& parser) {
        EXPECT_EQ("/soap", parser.getUrl());
        EXPECT_EQ("\"ServiceName#ActionName\"", parser.headerValue("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", parser.headerValue("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), parser.headerValue("Content-Length"));
        return fmt::format(response, body.size(), body);
    }));

    EXPECT_CALL(resMock, onResponse(std::make_error_code(http::error::Ok), body));

    client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope, handleResponse());
    io.run();
}

TEST_F(SoapClientTest, SoapActionWithFault)
{
    auto envelope = "data"s;

    auto body =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "<s:Body>"
        "<s:Fault>"
        "<faultcode>s:Client</faultcode>"
        "<faultstring>UPnPError</faultstring>"
        "<detail>"
        "<UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
        "<errorCode>718</errorCode>"
        "<errorDescription>ConflictInMappingEntry</errorDescription>"
        "</UPnPError>"
        "</detail>"
        "</s:Fault>"
        "</s:Body>"
        "</s:Envelope>"s;

    auto errorResponse =
        "HTTP/1.0 500 Internal Server Error\r\n"
        "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
        "CONTENT-LENGTH: {}\r\n"
        "\r\n"
        "{}";

    auto response = fmt::format(errorResponse, body.size(), body);

    EXPECT_CALL(reqMock, onRequest(_)).WillOnce(Invoke([&] (http::Parser& parser) {
        EXPECT_EQ("/soap", parser.getUrl());
        EXPECT_EQ("\"ServiceName#ActionName\"", parser.headerValue("SOAPACTION"));
        EXPECT_EQ("text/xml; charset=\"utf-8\"", parser.headerValue("Content-Type"));
        EXPECT_EQ(std::to_string(envelope.size()), parser.headerValue("Content-Length"));
        return response;
    }));

    EXPECT_CALL(resMock, onResponse(std::make_error_code(http::error::InternalServerError), body));

    client.action(server.getWebRootUrl() + "/soap", "ActionName", "ServiceName", envelope, handleResponse());
    io.run();
}

}
}
