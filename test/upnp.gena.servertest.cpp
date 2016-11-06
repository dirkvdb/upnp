#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp.gena.server.h"

namespace upnp
{
namespace test
{

using namespace asio;
using namespace testing;
using namespace std::placeholders;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string okResponse =
    "HTTP/1.1 200 OK\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: 41\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n"
    "<html><body><h1>200 OK</h1></body></html>";

static const std::string errorResponse =
    "HTTP/1.1 400 Bad request\r\n"
    "SERVER: Darwin/15.4.0, UPnP/1.0\r\n"
    "CONTENT-LENGTH: 50\r\n"
    "CONTENT-TYPE: text/html\r\n"
    "\r\n"
    "<html><body><h1>400 Bad request</h1></body></html>";

struct SubscriptionCallbackMock
{
    MOCK_METHOD1(onEvent, void(const SubscriptionEvent& ev));
};

class GenaServerTest : public Test
{
public:
    GenaServerTest()
    : client(io)
    , server(io, asio::ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 0), std::bind(&SubscriptionCallbackMock::onEvent, &cbMock, _1))
    {
    }

    void sendDataAndExpectResponse(const std::string& data, const std::string& expectedResponse)
    {
        client.async_connect(server.getAddress(), [&] (const asio_error_code& error) {
            EXPECT_FALSE(error);
            async_write(client, buffer(data), [&] (const asio_error_code& error, size_t) {
                EXPECT_FALSE(error);
                client.async_receive(buffer(buf), [&] (const asio_error_code& error, size_t size) {
                    EXPECT_FALSE(error);
                    EXPECT_GT(size, 0u);
                    EXPECT_EQ(expectedResponse, std::string(buf.data(), size));
                    client.close();
                    server.stop();
                });
            });
        });
    }

    SubscriptionCallbackMock cbMock;
    io_service io;
    ip::tcp::socket client;
    gena::Server server;

    std::array<char, 1024> buf;
};

TEST_F(GenaServerTest, Event)
{
    auto notification =
        "NOTIFY / HTTP/1.1\r\n"
        "HOST: delivery host:delivery port\r\n"
        "CONTENT-TYPE: text/xml\r\n"
        "CONTENT-LENGTH: 21\r\n"
        "NT: upnp:event\r\n"
        "NTS: upnp:propchange\r\n"
        "SID: uuid:subscription-UUID\r\n"
        "SEQ: 5\r\n"
        "\r\n"
        "<?xml version=\"1.0\"?>"s;

    EXPECT_CALL(cbMock, onEvent(_)).WillOnce(Invoke([] (const SubscriptionEvent& ev) {
        EXPECT_EQ(5u, ev.sequence);
        EXPECT_EQ("uuid:subscription-UUID"s, ev.sid);
        EXPECT_EQ("<?xml version=\"1.0\"?>"s, ev.data);
    }));

    sendDataAndExpectResponse(notification, okResponse);
    io.run();
}

TEST_F(GenaServerTest, EventBadRequest)
{
    auto notification =
        "NOTIFY / HTTP/1.1\r\n"
        "HOST: delivery host:delivery port\r\n"
        "CONTENT-TYPE: text/xml\r\n"
        "CONTENT-LENGTH: 21\r\n"
        "CONNECTION: close\r\n"
        "NT: upnp:evvent\r\n" // bad event type
        "NTS: upnp:propchange\r\n"
        "SID: uuid:subscription-UUID\r\n"
        "SEQ: 5\r\n"
        "\r\n"
        "<?xml version=\"1.0\"?>"s;

    sendDataAndExpectResponse(notification, errorResponse);
    io.run();
}

TEST_F(GenaServerTest, ExptectConnectionClose)
{
    auto notification =
        "NOTIFY / HTTP/1.1\r\n"
        "HOST: delivery host:delivery port\r\n"
        "CONTENT-TYPE: text/xml\r\n"
        "CONTENT-LENGTH: 21\r\n"
        "CONNECTION: close\r\n"
        "NT: upnp:event\r\n"
        "NTS: upnp:propchange\r\n"
        "SID: uuid:subscription-UUID\r\n"
        "SEQ: 5\r\n"
        "\r\n"
        "<?xml version=\"1.0\"?>"s;

    EXPECT_CALL(cbMock, onEvent(_)).WillOnce(Invoke([] (const SubscriptionEvent& ev) {
        EXPECT_EQ(5u, ev.sequence);
        EXPECT_EQ("uuid:subscription-UUID"s, ev.sid);
        EXPECT_EQ("<?xml version=\"1.0\"?>"s, ev.data);
    }));

    client.async_connect(server.getAddress(), [&] (const asio_error_code& error) {
        EXPECT_FALSE(error);
        async_write(client, buffer(notification), [&] (const asio_error_code& error, size_t) {
            EXPECT_FALSE(error);
            client.async_receive(buffer(buf), [&] (const asio_error_code& error, ssize_t size) {
                EXPECT_FALSE(error);
                // First read contains the response
                EXPECT_GT(size, 0);
                EXPECT_EQ(okResponse, std::string(buf.data(), size));

                client.async_receive(buffer(buf), [&] (const asio_error_code& error, ssize_t size) {
                    // Second read should be the connection close
                    EXPECT_EQ(asio::error::eof, error.value());
                    EXPECT_EQ(0, size);
                    // So also close our connection
                    client.close();
                    server.stop();
                });
            });
        });
    });

    io.run();
}

}
}
