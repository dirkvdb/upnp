#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/log.h"
#include "upnp.gena.server.h"

namespace upnp
{
namespace test
{

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
    : client(loop)
    , server(loop, uv::Address::createIp4("127.0.0.1", 0), std::bind(&SubscriptionCallbackMock::onEvent, &cbMock, _1))
    {
    }
    
    void sendDataAndExpectResponse(const std::string& data, const std::string& expectedResponse)
    {
        client.connect(server.getAddress(), [&] (int32_t status) {
            EXPECT_EQ(0, status);
            client.write(uv::Buffer(data, uv::Buffer::Ownership::No), [&] (int32_t status) {
                EXPECT_EQ(0, status);
                client.read([&] (ssize_t size, const uv::Buffer& buf) {
                    EXPECT_TRUE(size > 0);
                    EXPECT_EQ(expectedResponse, std::string(buf.data(), size));

                    client.close([this] () {
                        server.stop(nullptr);
                    });
                });
            });
        });
    }

    SubscriptionCallbackMock cbMock;
    uv::Loop loop;
    uv::socket::Tcp client;
    gena::Server server;
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
    loop.run(uv::RunMode::Default);
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
    loop.run(uv::RunMode::Default);
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

    uint32_t readCount = 0;
    client.connect(server.getAddress(), [&] (int32_t status) {
        EXPECT_EQ(0, status);
        client.write(uv::Buffer(notification, uv::Buffer::Ownership::No), [&] (int32_t status) {
            EXPECT_EQ(0, status);
            client.read([&] (ssize_t size, const uv::Buffer& buf) {
                if (readCount == 0)
                {
                    // First read contains the response
                    EXPECT_TRUE(size > 0);
                    EXPECT_EQ(okResponse, std::string(buf.data(), size));
                }
                else
                {
                    // Second read should be the connection close
                    EXPECT_EQ(UV_EOF, size);
                    // So also close our connection
                    client.close([this] () {
                        server.stop(nullptr);
                    });
                }

                ++readCount;
            });
        });
    });

    loop.run(uv::RunMode::Default);
}

}
}
