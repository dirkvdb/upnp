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

    // Search for any device that provides the serviceType
    void search(const std::string& serviceType);
    // Search for a specific device that provides the serviceType
    void search(const std::string& serviceType, const std::string& deviceIp);
    void stop();

    void setDiscoverCallback(std::function<void(std::string)> cb);

private:
    void run();

    uv::socket::Udp m_socket;
    std::function<void(std::string)> m_cb;
};

}
}
