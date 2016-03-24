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
    Server(uv::Loop& loop)
    : m_socket(loop)
    {
        run();
    }

private:
    void run();

    uv::socket::Udp m_socket;
};

}
}