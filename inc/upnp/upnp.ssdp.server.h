#pragma once

#include <string>
#include <memory>
#include <chrono>

#include "upnp/upnp.uv.h"
#include "upnp/upnp.device.h"

namespace upnp
{

namespace ssdp
{

class SearchParser;

class Server
{
public:
    Server(uv::Loop& loop);
    ~Server() noexcept;

    void run(const Device& info);
    void stop(std::function<void()> cb);

private:
    void sendMessages(const std::vector<std::string>& msgs, std::shared_ptr<uv::Timer> timer, int32_t count);

    void announceDevice();
    void announceDeviceStop(std::function<void(int32_t)> cb);

    void respondToSearch(const std::string& host, const std::string& searchTarget, std::chrono::seconds delay, const uv::Address& addr);
    void sendResponse(std::shared_ptr<std::string> response, std::shared_ptr<uv::Timer> timer, const uv::Address& addr);

    uv::Loop& m_loop;
    uv::Timer m_timer;
    uv::socket::Udp m_socket;
    std::unique_ptr<SearchParser> m_parser;

    std::vector<std::string> m_announceMessages;
    std::vector<std::string> m_byebyeMessages;

    Device m_device;
};

}
}
