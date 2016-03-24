#include "gtest/gtest.h"

#include "utils/log.h"
#include "upnp/upnpssdpserver.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::chrono_literals;

class SsdpServerTest : public Test
{
protected:
    SsdpServerTest()
    : m_server(m_loop)
    {
    }

    ~SsdpServerTest()
    {
        utils::log::info("~SsdpServerTest");
    }

    uv::Loop        m_loop;
    ssdp::Server    m_server;
};

TEST_F(SsdpServerTest, SsdpServer)
{
    //utils::log::info("SsdpServer");
    //m_loop.run(uv::RunMode::Default);
}

TEST_F(SsdpServerTest, TimerTestOnce)
{
    uv::Timer t(m_loop);

    t.start(50ms, 0ms, [=] () {
        utils::log::info("Timer done");
    });

    m_loop.run(uv::RunMode::Once);
}

TEST_F(SsdpServerTest, TimerTest)
{
    uv::Timer t(m_loop);

    int count = 0;

    t.start(50ms, 10ms, [&] () {
        utils::log::info("Timer done");

        if (++count == 3)
        {
            t.stop();
            stopLoopAndCloseRequests(m_loop);
        }
    });

    m_loop.run(uv::RunMode::Default);
}

}
}
