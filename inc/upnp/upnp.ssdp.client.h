#pragma once

#include <string>

#include "upnp/upnp.uv.h"

namespace upnp
{
namespace ssdp
{

class Parser;

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
    void stop(std::function<void()> cb);

    void setSearchTimeout(std::chrono::seconds timeout);

    // Multicast Search for any device that provides any service type
    void search();
    // Multicast Search for any device that provides the serviceType
    void search(const char* serviceType);
    // Unicast Search for a specific device that provides the serviceType
    void search(const char* serviceType, const char* deviceIp);

    void setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb);

private:
    void parseData();
    void sendMessages(const uv::Address& addr, std::shared_ptr<std::string> content);

    uint32_t m_searchTimeout;
    uv::Loop& m_loop;
    uv::socket::Udp m_socket;
    std::unique_ptr<Parser> m_parser;
};

}
}
