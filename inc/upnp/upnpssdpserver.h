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

    void search();
    void setDiscoverCallback(std::function<void(std::string)> cb);
    void stop();

private:
    void run();

    uv::socket::Udp m_socket;
    std::function<void(std::string)> m_cb;
};

}
}
