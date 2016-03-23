#pragma once

#include <string>

#include "upnp/upnpuv.h"

namespace upnp
{

class Loop;

namespace ssdp
{

class Server
{
public:
    template <typename LoopType>
    Server(LoopType& loop)
    : m_socket(loop, "0.0.0.0", 0, static_cast<uint32_t>(socket::UdpFlags::ReuseAddress))
    {
        run();
    }

private:
    void run();

    socket::Udp m_socket;
};

}
}