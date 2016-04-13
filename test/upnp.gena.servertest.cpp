#include <catch.hpp>

#include "utils/log.h"
#include "upnp.gena.server.h"

namespace upnp
{
namespace test
{

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

auto checkResponseOk = [] (const std::string& response) {
    CHECK(response == okResponse);
};

auto checkResponseError = [] (const std::string& response) {
    CHECK(response == errorResponse);
};

TEST_CASE("Gena server", "[gena]")
{
    uv::Loop loop;
    uv::socket::Tcp client(loop);
    SubscriptionEvent event;

    std::string notification;
    std::function<void(const std::string&)> checkResponse;
    std::function<void(const SubscriptionEvent&)> checkEvent;

    auto server = std::make_unique<gena::Server>(loop, uv::Address::createIp4("127.0.0.1", 0), [&] (const SubscriptionEvent& ev) {
        checkEvent(ev);
    });

    SECTION("event")
    {
        notification =
        "NOTIFY / HTTP/1.1\r\n"
        "HOST: delivery host:delivery port\r\n"
        "CONTENT-TYPE: text/xml\r\n"
        "CONTENT-LENGTH: 21\r\n"
        "NT: upnp:event\r\n"
        "NTS: upnp:propchange\r\n"
        "SID: uuid:subscription-UUID\r\n"
        "SEQ: 5\r\n"
        "\r\n"
        "<?xml version=\"1.0\"?>";

        checkEvent = [] (const SubscriptionEvent& ev) {
            CHECK(ev.sequence == 5);
            CHECK(ev.sid == "uuid:subscription-UUID");
            CHECK(ev.data == "<?xml version=\"1.0\"?>");
        };

        checkResponse = checkResponseOk;
    }

    SECTION("event bad request")
    {
        notification =
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
        "<?xml version=\"1.0\"?>";

        checkResponse = checkResponseError;
    }

    client.connect(server->getAddress(), [&] (int32_t status) {
        CHECK(status == 0);
        client.write(uv::Buffer(notification, uv::Buffer::Ownership::No), [&] (int32_t status) {
            CHECK(status == 0);
            client.read([&] (ssize_t size, const uv::Buffer& buf) {
                CHECK(size > 0);
                checkResponse(std::string(buf.data(), size));

                client.close([&server] () {
                    server->stop(nullptr);
                });
            });
        });
    });

    loop.run(uv::RunMode::Default);
}

TEST_CASE("Gena server expect connection close", "[gena]")
{
    uv::Loop loop;
    uv::socket::Tcp client(loop);
    SubscriptionEvent event;

    std::string notification =
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
        "<?xml version=\"1.0\"?>";

    auto server = std::make_unique<gena::Server>(loop, uv::Address::createIp4("127.0.0.1", 0), [&] (const SubscriptionEvent& ev) {
        CHECK(ev.sequence == 5);
        CHECK(ev.sid == "uuid:subscription-UUID");
        CHECK(ev.data == "<?xml version=\"1.0\"?>");
    });

    uint32_t readCount = 0;
    client.connect(server->getAddress(), [&] (int32_t status) {
        CHECK(status == 0);
        client.write(uv::Buffer(notification, uv::Buffer::Ownership::No), [&] (int32_t status) {
            CHECK(status == 0);
            client.read([&] (ssize_t size, const uv::Buffer& buf) {
                if (readCount == 0)
                {
                    // First read contains the response
                    CHECK(size > 0);
                    CHECK(std::string(buf.data(), size) == okResponse);
                }
                else
                {
                    // Second read should be the connection close
                    CHECK(size == UV_EOF);
                    // So also close our connection
                    client.close([&server] () {
                        server->stop(nullptr);
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
