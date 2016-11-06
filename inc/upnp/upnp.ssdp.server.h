#pragma once

#include <string>
#include <memory>
#include <chrono>

#include "upnp/asio.h"

#include "upnp/upnp.device.h"

namespace upnp
{

namespace ssdp
{

class Server
{
public:
    Server(asio::io_service& io);
    ~Server() noexcept;

    void run(const Device& info, std::chrono::seconds announceInterval);
    void stop(std::function<void()> cb);

private:
    void sendMessages(const std::vector<std::string>& msgs, std::shared_ptr<asio::steady_timer> timer, int32_t count);

    void announceDevice();
    void announceDeviceStop(std::function<void(const asio_error_code&, size_t)> cb);

    void respondToSearch(const std::string& host, const std::string& searchTarget, std::chrono::seconds delay, const asio::ip::udp::endpoint& addr);
    void sendResponse(std::shared_ptr<std::string> response, std::shared_ptr<asio::steady_timer> timer, const asio::ip::udp::endpoint& addr);

    bool isResponseNeeded(const std::string& searchTarget);

    void receiveData();
    void onDataReceived(const std::error_code& error, size_t bytesReceived);

    asio::io_service& m_io;
    asio::steady_timer m_timer;
    asio::steady_timer m_announceTimer;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::socket m_unicastSocket;
    asio::ip::udp::endpoint m_sender;

    std::vector<std::string> m_announceMessages;
    std::vector<std::string> m_byebyeMessages;
    std::array<char, 1024> m_buffer;
    std::chrono::seconds m_announceInterval;

    Device m_device;
};

}
}
