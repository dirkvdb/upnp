#include "upnp/upnp.ssdp.server.h"
#include "upnp/upnp.device.h"
#include "upnp.ssdp.parseutils.h"


#include "utils/log.h"
#include "utils/stringoperations.h"

#include <random>

namespace upnp
{
namespace ssdp
{

using namespace utils;

static const char* s_ssdpIp = "239.255.255.250";
static const std::string s_anyAddress = "0.0.0.0";
static const int32_t s_ssdpPort = 1900;
static const uint32_t s_broadcastRepeatCount = 3;
static const uv::Address s_ssdpAddressIpv4 = uv::Address::createIp4(s_ssdpIp, s_ssdpPort);

static const std::string s_aliveNotification =
"NOTIFY * HTTP/1.1r\r\n"
"HOST:239.255.255.250:1900\r\n"
"CACHE-CONTROL:max-age=1800\r\n"
"LOCATION:{}\r\n"
"SERVER: UPnP/1.1 1.0\r\n"
"NT:upnp:rootdevice\r\n"
"NTS:ssdp:alive\r\n"
"USN:{}\r\n"
"BOOTID.UPNP.ORG: 1\r\n"
"CONFIGID.UPNP.ORG: 1\r\n"
"\r\n";

static const std::string s_byebyeNotification =
"NOTIFY * HTTP/1.1\r\n"
"HOST:239.255.255.250:1900\r\n"
"NT:upnp:rootdevice\r\n"
"NTS:ssdp:byebye\r\n"
"USN:{}\r\n"
"BOOTID.UPNP.ORG: 1\r\n"
"CONFIGID.UPNP.ORG: number used for caching description information\r\n"
"\r\n";

Server::Server(uv::Loop& loop)
: m_loop(loop)
, m_timer(loop)
, m_socket(loop)
, m_parser(std::make_unique<SearchParser>())
{
    //m_parser->setSearchRequestCallback([this] (auto& st, auto& delay) { respondToSearch(st, delay); });
}

Server::~Server() noexcept = default;

void Server::run(const Device& info)
{
    // join the multicast channel
    m_socket.setBroadcast(true);
    m_socket.setTtl(2);
    m_socket.setMemberShip(s_ssdpIp, uv::socket::Udp::MemberShip::JoinGroup);

    m_socket.recv([=] (const std::string& msg) {
        try
        {
            auto parsed = m_parser->parse(msg);
            assert(parsed == msg.size());
        }
        catch (std::exception& e)
        {
            log::warn("Failed to parse http notification: {}", e.what());
        }
    });

    announceDevice(info);
}

void Server::stop(std::function<void()> cb)
{
    m_socket.close(std::move(cb));
}

void Server::sendAnnounceMessage(std::shared_ptr<std::string> msg, std::shared_ptr<uv::Timer> timer, int32_t count)
{
    m_socket.send(s_ssdpAddressIpv4, *msg, [this, msg, timer, count] (int32_t status) {
        if (status < 0)
        {
            log::warn("Failed to send broadcast message");
        }

        if ((count - 1) > 0)
        {
            static std::default_random_engine re;
            static std::uniform_int_distribution<> dis(50, 200);

            m_timer.start(std::chrono::milliseconds(dis(re)), [this, msg, timer, count] () {
                sendAnnounceMessage(msg, timer, count - 1);
            });
        }
    });
}

void Server::announceDevice(const Device& info)
{
    auto msg = std::make_shared<std::string>(fmt::format(s_aliveNotification, info.location, info.udn));
    auto timer = std::make_shared<uv::Timer>(m_loop);

    sendAnnounceMessage(msg, timer, s_broadcastRepeatCount);
}

void Server::respondToSearch(const std::string& searchTarget, std::chrono::seconds delay)
{
    log::info("Search request: {} {}", searchTarget, delay.count());
}

}
}
