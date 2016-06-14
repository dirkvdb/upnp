#pragma once

#include <array>
#include <string>

#include <asio.hpp>

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
    Client(asio::io_service& svc);
    ~Client() noexcept;

    void run();
    void run(const std::string& address);
    void stop();

    void setSearchTimeout(std::chrono::seconds timeout);

    // Multicast Search for any device that provides any service type
    void search();
    // Multicast Search for any device that provides the serviceType
    void search(const char* serviceType);
    // Unicast Search for a specific device that provides the serviceType
    void search(const char* serviceType, const char* deviceIp);

    void setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb);

private:
    void run(const asio::ip::udp::endpoint& addr);

    void parseData();
    void sendMessages(const asio::ip::udp::endpoint& addr, std::shared_ptr<std::string> content);

    uint32_t m_searchTimeout;
    asio::io_service& m_service;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_sender;
    std::unique_ptr<Parser> m_parser;
    std::array<char, 1024> m_buffer;
};

}
}
