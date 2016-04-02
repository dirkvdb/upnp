#include <catch.hpp>

#include "utils/log.h"
#include "upnp/upnp.uv.h"

namespace upnp
{
namespace test
{

using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("UV Test", "[uv]")
{
    uv::Loop loop;

    SECTION("TimerTestOnce", "[uv]")
    {
        uv::Timer t(loop);
        bool timerFinished = false;

        t.start(50ms, 0ms, [&] () {
            timerFinished = true;
        });

        loop.run(uv::RunMode::Once);
        CHECK(timerFinished);
    }

    SECTION("TimerTest", "[uv]")
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
    
    SECTION("TcpTest", "[uv]")
    {
        uv::socket::Tcp server(loop);
        uv::socket::Tcp client(loop);
        
        auto data = "HELLO"s;

        server.bind(uv::Address::createIp4("127.0.0.1", 8080));
        server.listen(128, [&] (int32_t status) {
            CHECK(status == 0);
            
            auto s = new uv::socket::Tcp(loop);
            server.accept(*s);
            s->read([&] (ssize_t count, const uv::Buffer& buf) {
                if (count > 0)
                {
                    CHECK(count == data.size());
                    CHECK(memcmp(data.data(), buf.data(), count) == 0);
                }
                else
                {
                    CHECK(count == UV_EOF);
                    server.close(nullptr);
                    delete s;
                }
            });
        });
        
        client.connect(uv::Address::createIp4("127.0.0.1", 8080), [&] (int32_t status) {
            CHECK(status == 0);
            client.write(uv::Buffer(&data.front(), data.size()), [&] (int32_t status) {
                CHECK(status == 0);
                client.close(nullptr);
            });
        });

        loop.run(uv::RunMode::Default);
    }
    
    SECTION("TcpTest modified port", "[uv]")
    {
        uv::socket::Tcp server(loop);
        uv::socket::Tcp client(loop);
        
        auto data = "HELLO"s;
        
        ;
        
        auto addr = uv::Address::createIp4(uv::createIp4Address("127.0.0.1", 80));
        addr.setPort(8080);

        server.bind(addr);
        server.listen(128, [&] (int32_t status) {
            CHECK(status == 0);
            
            auto s = new uv::socket::Tcp(loop);
            server.accept(*s);
            s->read([&] (ssize_t count, const uv::Buffer& buf) {
                if (count > 0)
                {
                    CHECK(count == data.size());
                    CHECK(memcmp(data.data(), buf.data(), count) == 0);
                }
                else
                {
                    CHECK(count == UV_EOF);
                    server.close(nullptr);
                    delete s;
                }
            });
        });
        
        client.connect(server.getSocketName(), [&] (int32_t status) {
            CHECK(status == 0);
            client.write(uv::Buffer(&data.front(), data.size()), [&] (int32_t status) {
                CHECK(status == 0);
                client.close(nullptr);
            });
        });

        loop.run(uv::RunMode::Default);
    }
}

TEST_CASE("List interfaces", "[uv]")
{
    for (auto& addr : uv::getNetworkInterfaces())
    {
        std::cout << "Name: " << addr.name << std::endl
                  << "Addr: " << addr.ipName() << std::endl
                  << "Internal:" << addr.isInternal << std::endl;
    }
}

}
}
