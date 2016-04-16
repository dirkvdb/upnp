#include "upnp/upnp.ssdp.client.h"

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.ssdp.parseutils.h"

namespace upnp
{
namespace ssdp
{

using namespace utils;

static const std::string g_ssdpIp = "239.255.255.250";
static const std::string g_anyAddress = "0.0.0.0";
static const int32_t g_ssdpPort = 1900;
static const uint32_t g_broadcastRepeatCount = 5;
static const uv::Address g_ssdpAddressIpv4 = uv::Address::createIp4(g_ssdpIp, g_ssdpPort);

Client::Client(uv::Loop& loop)
: m_searchTimeout(3)
, m_socket(loop)
, m_parser(std::make_unique<Parser>())
{
}

Client::~Client() noexcept = default;

void Client::run()
{
    run(g_anyAddress);
}

void Client::run(const std::string& address)
{
    m_socket.bind(uv::Address::createIp4(address, 0), uv::socket::UdpFlag::ReuseAddress);

    // join the multicast channel
    m_socket.setBroadcast(true);
    m_socket.setMemberShip(g_ssdpIp, uv::socket::Udp::MemberShip::JoinGroup);
    m_socket.setTtl(4);

    m_socket.recv([=] (const std::string& msg) {
        try
        {
            //utils::log::debug("Read {}", msg);
            auto parsed = m_parser->parse(msg);
            assert(parsed == msg.size());
        }
        catch (std::exception& e)
        {
            log::warn("Failed to parse http notification: {}", e.what());
        }
    });
}

void Client::stop()
{
    m_socket.close([] () {
        log::debug("SSDP client socket closed");
    });
}

void Client::setSearchTimeout(std::chrono::seconds timeout)
{
    m_searchTimeout = timeout.count();
}

void Client::search()
{
    search("ssdp:all");
}

void Client::search(const std::string& serviceType)
{
    search(serviceType, g_ssdpIp);
}

void Client::search(const std::string& serviceType, const std::string& deviceIp)
{
    std::string req = fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                  "HOST:{}:{}\r\n"
                                  "MAN:\"ssdp:discover\"\r\n"
                                  "MX:{}\r\n"
                                  "ST:{}\r\n"
                                  "\r\n", deviceIp, g_ssdpPort, m_searchTimeout, serviceType);

    for (uint32_t i = 0; i < g_broadcastRepeatCount; ++i)
    {
        m_socket.send(g_ssdpAddressIpv4, req, [] (int32_t status) {
            log::info("Send completed {}", status);
        });
    }
}

void Client::setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb)
{
    m_parser->setHeaderParsedCallback(std::move(cb));
}

}
}
