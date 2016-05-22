#pragma once

#include <string>
#include <memory>
#include <chrono>

#include "upnp/upnp.uv.h"

namespace upnp
{

struct Device;

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

    void respondToSearch(const std::string& searchTarget, std::chrono::seconds delay);

    uv::Loop& m_loop;
    uv::Timer m_timer;
    uv::socket::Udp m_socket;
    std::unique_ptr<SearchParser> m_parser;

    std::vector<std::string> m_announceMessages;
    std::vector<std::string> m_byebyeMessages;
};

}
}
