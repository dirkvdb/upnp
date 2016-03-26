#include "upnp/upnpssdpserver.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp.upnp.ssdp.parseutils.h"

namespace upnp
{
namespace ssdp
{

static const std::string g_ssdpIp = "239.255.255.250";
static const std::string g_anyAddress = "0.0.0.0";
static const int32_t g_ssdpPort = 1900;
static const uint32_t g_broadcastRepeatCount = 5;
static const sockaddr_in g_ssdpAddressIpv4 = uv::createIp4Address(g_ssdpIp, g_ssdpPort);

void Server::run()
{
    m_socket.bind("10.160.6.84", g_ssdpPort, uv::socket::UdpFlag::ReuseAddress);
    m_socket.setBroadcast(true);

    // join the multicast channel
    m_socket.setMemberShip(g_ssdpIp, uv::socket::Udp::MemberShip::JoinGroup);

    m_socket.recv([=] (const std::string& msg) {
        if (msg.empty())
        {
            utils::log::info("Read done");
            return;
        }

        utils::log::info("Read {}", msg);
        if (m_cb && utils::stringops::startsWith(msg, "NOTIFY"))
        {
            m_cb(msg);
        }
    });
}

void Server::stop()
{

}

void Server::search(const std::string& serviceType)
{
    search(serviceType, g_ssdpIp);
}

void Server::search(const std::string& serviceType, const std::string& deviceIp)
{
    std::string req = fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                  "HOST:{}:{}\r\n"
                                  "MAN:\"ssdp:discover\"\r\n"
                                  "MX:3\r\n"
                                  "ST:{}\r\n"
                                  "\r\n", deviceIp, g_ssdpPort, serviceType);

    for (uint32_t i = 0; i < g_broadcastRepeatCount; ++i)
    {
        m_socket.send(g_ssdpAddressIpv4, req, [] (int32_t status) {
            utils::log::info("Send completed {}", status);
        });
    }
}

void Server::setDiscoverCallback(std::function<void(std::string)> cb)
{
    m_cb = std::move(cb);
}

}
}
