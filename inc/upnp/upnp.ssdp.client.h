#pragma once

#include <array>
#include <string>

#include <asio.hpp>

namespace upnp
{
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
    Client(asio::io_service& io);

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
    struct Receiver;

    static void receiveData(const std::shared_ptr<Receiver>& receiver);
    static void sendMessages(asio::ip::udp::socket& sock, const asio::ip::udp::endpoint& addr, std::shared_ptr<std::string> content);

    void run(const asio::ip::udp::endpoint& addr);
    void parseData();

    std::chrono::seconds m_searchTimeout;

    std::shared_ptr<Receiver> m_unicast;
    std::shared_ptr<Receiver> m_multicast;
};

}
}
