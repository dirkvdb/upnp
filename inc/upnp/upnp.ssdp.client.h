#pragma once

#include <string>

#include "upnp/upnp.uv.h"

namespace upnp
{

namespace http { class Parser; }

namespace ssdp
{

enum class NotificationType
{
    Alive,
    ByeBye
};

struct DeviceNotificationInfo
{
    uint32_t            expirationTime;
    std::string         deviceId;
    std::string         deviceType;
    std::string         serviceType;
    std::string         serviceVersion;
    std::string         location;
    NotificationType    type;
};

class Client
{
public:
    Client(uv::Loop& loop);
    ~Client() noexcept;

    void run();
    void run(const std::string& address);
    void stop();

    void setSearchTimeout(std::chrono::seconds timeout);

    // Search for any device that provides any service type
    void search();
    // Search for any device that provides the serviceType
    void search(const std::string& serviceType);
    // Search for a specific device that provides the serviceType
    void search(const std::string& serviceType, const std::string& deviceIp);

    void setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb);

private:
    void parseData();

    uint32_t m_searchTimeout;
    uv::socket::Udp m_socket;
    std::function<void(const DeviceNotificationInfo&)> m_cb;
    std::unique_ptr<http::Parser> m_parser;
};

}
}
