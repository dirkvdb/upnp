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
    void sendAnnounceMessage(std::shared_ptr<std::string> msg, std::shared_ptr<uv::Timer> timer, int32_t count);
    void announceDevice(const Device& info);

    void respondToSearch(const std::string& searchTarget, std::chrono::seconds delay);

    uv::Loop& m_loop;
    uv::Timer m_timer;
    uv::socket::Udp m_socket;
    std::unique_ptr<SearchParser> m_parser;
};

}
}
