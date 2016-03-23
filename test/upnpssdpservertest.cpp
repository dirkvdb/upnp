#include "gtest/gtest.h"

#include "utils/log.h"
#include "upnp/upnpssdpserver.h"

namespace upnp
{
namespace test
{

using namespace testing;

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

    DefaultLoop     m_loop;
    //Idler           m_idler;
    ssdp::Server    m_server;
};

TEST_F(SsdpServerTest, SsdpServer)
{
    utils::log::info("SsdpServer");
    m_loop.run();
}

}
}
