#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.uv.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

class UVTest : public Test
{
public:
    ~UVTest()
    {

    }

    uv::Loop loop;
};


TEST_F(UVTest, TimerTestOnce)
{
    uv::Timer t(loop);
    bool timerFinished = false;

    t.start(50ms, 0ms, [&] () {
        timerFinished = true;
    });

    loop.run(uv::RunMode::Once);
    EXPECT_TRUE(timerFinished);

    loop.run(uv::RunMode::Default);
}

TEST_F(UVTest, TimerTest)
{
    uv::Timer t(loop);

    int count = 0;

    t.start(50ms, 10ms, [&] () {
        if (++count == 3)
        {
            t.stop();
            stopLoopAndCloseRequests(loop);
        }
    });

    loop.run(uv::RunMode::Default);
}

TEST_F(UVTest, TcpTest)
{
    uv::socket::Tcp server(loop);
    uv::socket::Tcp client(loop);

    const auto data = "HELLO"s;

    server.bind(uv::Address::createIp4("127.0.0.1", 8888));
    server.listen(128, [&] (int32_t status) {
        EXPECT_EQ(0, status);

        auto s = new uv::socket::Tcp(loop);
        server.accept(*s);
        s->read([=, &server] (ssize_t count, const uv::Buffer& buf) {
            if (count > 0)
            {
                EXPECT_EQ(data.size(), count);
                EXPECT_EQ(0, memcmp(data.data(), buf.data(), count));
            }
            else
            {
                EXPECT_EQ(UV_EOF, count);
                s->close([&server, s] () {
                    server.close(nullptr);
                    delete s;
                });
            }
        });
    });

    client.connect(uv::Address::createIp4("127.0.0.1", 8888), [&] (int32_t status) {
        EXPECT_EQ(0, status);
        client.write(uv::Buffer(data, uv::Buffer::Ownership::No), [&] (int32_t status) {
            EXPECT_EQ(0, status);
            client.close(nullptr);
        });
    });

    loop.run(uv::RunMode::Default);
}

TEST_F(UVTest, TcpTestModifiedPort)
{
    uv::socket::Tcp server(loop);
    uv::socket::Tcp client(loop);

    const auto data = "HELLO"s;
    auto addr = uv::Address::createIp4(uv::createIp4Address("127.0.0.1", 80));
    addr.setPort(8888);

    server.bind(addr);
    server.listen(128, [&] (int32_t status) {
        EXPECT_EQ(0, status);

        auto s = new uv::socket::Tcp(loop);
        server.accept(*s);
        s->read([data, &server, s] (ssize_t count, const uv::Buffer& buf) {
            if (count > 0)
            {
                EXPECT_EQ(data.size(), count);
                EXPECT_EQ(0, memcmp(data.data(), buf.data(), count));
            }
            else
            {
                EXPECT_EQ(UV_EOF, count);
                s->close([&server, s] () {
                    server.close(nullptr);
                    delete s;
                });
            }
        });
    });

    client.connect(server.getSocketName(), [&] (int32_t status) {
        EXPECT_EQ(0, status);
        client.write(uv::Buffer(data, uv::Buffer::Ownership::No), [&] (int32_t status) {
            EXPECT_EQ(0, status);
            client.close(nullptr);
        });
    });

    loop.run(uv::RunMode::Default);
}

TEST_F(UVTest, ListInterfaces)
{
    for (auto& addr : uv::getNetworkInterfaces())
    {
        EXPECT_FALSE(addr.name.empty());
        EXPECT_FALSE(addr.ipName().empty());
    }
}

}
}
