#include <gtest/gtest.h>

#include "utils/log.h"
#include "upnp/upnp.coroutine.h"

#include <future>

namespace upnp
{
namespace test
{

using namespace asio;
using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

class CoroutineTest : public Test
{
public:
    CoroutineTest()
    {
    }

    io_service io;
};

TEST_F(CoroutineTest, connectCoro)
{
    Future<void> fut = [this] () -> Future<void> {
        auto addr = ip::tcp::endpoint(ip::address::from_string("8.8.8.8"), 53);
        ip::tcp::socket socket(io);
        socket.open(addr.protocol());
        co_await upnp::async_connect(socket, addr);
    }();

    while (!fut.await_ready())
    {
        io.run_one();
    }

    EXPECT_NO_THROW(fut.get());
}

}
}
