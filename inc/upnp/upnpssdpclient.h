#pragma once

#include <string>

#include "upnp/upnpuv.h"

namespace upnp
{

class Loop;

namespace ssdp
{

struct DeviceDiscoverInfo
{
    uint32_t        expirationTime;
    std::string     deviceId;
    std::string     deviceType;
    std::string     serviceType;
    std::string     serviceVersion;
    std::string     location;
};

class Client
{
public:
    Client(uv::Loop& loop);

    void run();
    void run(const std::string& interface);
    void stop();

    // Search for any device that provides the serviceType
    void search(const std::string& serviceType);
    // Search for a specific device that provides the serviceType
    void search(const std::string& serviceType, const std::string& deviceIp);

    void setDiscoverCallback(std::function<void(const DeviceDiscoverInfo&)> cb);

private:
    uv::socket::Udp m_socket;
    std::function<void(const DeviceDiscoverInfo&)> m_cb;
};

}
}
