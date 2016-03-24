#include "upnp/upnpssdpserver.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

namespace upnp
{
namespace ssdp
{

static const std::string g_ssdpAddress = "239.255.255.250";
static const int32_t g_ssdpPort = 1900;

void Server::run()
{
    m_socket.bind("0.0.0.0", g_ssdpPort, uv::socket::UdpFlag::ReuseAddress);

    // join the multicast channel
    m_socket.setMemberShip(g_ssdpAddress);

    m_socket.read([=] (const std::string& msg) {
        if (msg.empty())
        {
            utils::log::info("Read done");
            return;
        }

        if (m_cb && utils::stringops::startsWith(msg, "NOTIFY"))
        {
            m_cb(msg);
        }
    });
}

void Server::search()
{
    std::string req = "M-SEARCH * HTTP/1.1\r\n"
                      "HOST: 192.168.1.219\r\n";

    m_socket.send(req, g_ssdpAddress, g_ssdpPort);
}

void Server::setDiscoverCallback(std::function<void(std::string)> cb)
{
    m_cb = std::move(cb);
}

void Server::stop()
{

}

}
}
