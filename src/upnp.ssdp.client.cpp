#include "upnp/upnp.ssdp.client.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.ssdp.parseutils.h"

namespace upnp
{
namespace ssdp
{

using namespace utils;
using namespace asio;
using namespace std::placeholders;

static const auto s_ssdpIp = ip::address_v4::from_string("239.255.255.250");
static const int16_t s_ssdpPort = 1900;
static const uint32_t s_broadcastRepeatCount = 5;
static const auto s_ssdpAddressIpv4 = ip::udp::endpoint(s_ssdpIp, s_ssdpPort);

struct Client::Receiver
{
    Receiver(io_service& io)
    : socket(io)
    {
    }

    ip::udp::socket socket;
    ip::udp::endpoint sender;
    Parser parser;
    std::array<char, 1024> buffer;
};

Client::Client(io_service& io)
: m_searchTimeout(3)
, m_unicast(std::make_shared<Receiver>(io))
, m_multicast(std::make_shared<Receiver>(io))
{
}

void Client::run()
{
    run(ip::udp::endpoint(ip::udp::v4(), s_ssdpPort));
}

void Client::run(const std::string& address)
{
    run(ip::udp::endpoint(ip::address_v4::from_string(address), s_ssdpPort));
}

void Client::run(const ip::udp::endpoint& addr)
{
    m_unicast->socket.open(addr.protocol());
    m_unicast->socket.set_option(ip::multicast::enable_loopback(true));
    m_unicast->socket.set_option(ip::multicast::hops(4));
    m_unicast->socket.bind(ip::udp::endpoint(addr.address(), 0));

    m_multicast->socket.open(addr.protocol());
    m_multicast->socket.set_option(ip::udp::socket::reuse_address(true));
    m_multicast->socket.set_option(ip::udp::socket::broadcast(true));
    m_multicast->socket.set_option(ip::multicast::enable_loopback(true));
    m_multicast->socket.set_option(ip::multicast::hops(4));
    m_multicast->socket.bind(addr);
    m_multicast->socket.set_option(ip::multicast::join_group(s_ssdpIp));

    receiveData(m_unicast);
    receiveData(m_multicast);
}

void Client::stop()
{
    boost::system::error_code error;
    m_multicast->socket.close(error);
    m_unicast->socket.close(error);
}

void Client::receiveData(const std::shared_ptr<Receiver>& receiver)
{
    receiver->socket.async_receive_from(buffer(receiver->buffer), receiver->sender, [receiver] (const boost::system::error_code& error, size_t bytesReceived) {
        if (error)
        {
            receiver->parser.reset();

            if (receiver->socket.is_open())
            {
                receiveData(receiver);
            }

            return;
        }

        try
        {
            if (bytesReceived > 0)
            {
                //log::info("SSDP client msg received: {}", receiver->buffer.data(), bytesReceived);
                receiver->parser.parse(receiver->buffer.data(), bytesReceived);
            }
        }
        catch (std::exception& e)
        {
            log::warn("Error parsing http notification: {}", e.what());
        }

        receiver->parser.reset();
        receiveData(receiver);
    });
}

void Client::setSearchTimeout(std::chrono::seconds timeout)
{
    m_searchTimeout = timeout;
}

void Client::search()
{
    search("ssdp:all");
}

void Client::search(const char* serviceType)
{
    auto req = std::make_shared<std::string>(fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                                         "HOST:{}:{}\r\n"
                                                         "MAN:\"ssdp:discover\"\r\n"
                                                         "MX:{}\r\n"
                                                         "ST:{}\r\n"
                                                         "\r\n", s_ssdpIp, s_ssdpPort, m_searchTimeout.count(), serviceType));

    sendMessages(m_unicast->socket, s_ssdpAddressIpv4, req);
}

void Client::search(const char* serviceType, const char* deviceIp)
{
    auto addr = m_unicast->socket.local_endpoint();
    auto req = std::make_shared<std::string>(fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                                         "HOST:{}:{}\r\n"
                                                         "MAN:\"ssdp:discover\"\r\n"
                                                         "ST:{}\r\n"
                                                         "\r\n", "192.168.1.10", addr.port(), serviceType));

    sendMessages(m_unicast->socket, ip::udp::endpoint(ip::address_v4::from_string(deviceIp), s_ssdpPort), req);
}

void Client::sendMessages(asio::ip::udp::socket& sock, const asio::ip::udp::endpoint& addr, std::shared_ptr<std::string> content)
{
    for (uint32_t i = 0; i < s_broadcastRepeatCount; ++i)
    {
        sock.async_send_to(buffer(*content), addr, [content] (const boost::system::error_code& error, size_t) {
            if (error)
            {
                log::warn("Ssdp search failed: {}", error.message());
            }
        });
    }
}

void Client::setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb)
{
    m_unicast->parser.setHeaderParsedCallback(cb);
    m_multicast->parser.setHeaderParsedCallback(cb);
}

}
}
