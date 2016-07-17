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

struct Client::Pimpl
{
    Pimpl(asio::io_service& io)
    : searchTimeout(3)
    , service(io)
    , socket(io)
    , unicastSocket(io)
    {
    }

    uint32_t searchTimeout;
    asio::io_service& service;
    asio::ip::udp::socket socket;
    asio::ip::udp::socket unicastSocket;
    asio::ip::udp::endpoint sender;
    Parser parser;
    std::array<char, 1024> buffer;
};

Client::Client(io_service& io)
: m_pimpl(std::make_shared<Pimpl>(io))
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

void Client::run(const asio::ip::udp::endpoint& addr)
{
    m_pimpl->unicastSocket.open(addr.protocol());
    m_pimpl->unicastSocket.set_option(ip::multicast::enable_loopback(true));
    m_pimpl->unicastSocket.set_option(ip::multicast::hops(4));

    m_pimpl->socket.open(addr.protocol());
    m_pimpl->socket.set_option(ip::udp::socket::reuse_address(true));
    m_pimpl->socket.set_option(ip::udp::socket::broadcast(true));
    m_pimpl->socket.set_option(ip::multicast::enable_loopback(true));
    m_pimpl->socket.set_option(ip::multicast::hops(4));
    
    m_pimpl->socket.bind(addr);
    
    // join the multicast channel
    m_pimpl->socket.set_option(ip::multicast::join_group(s_ssdpIp));

    receiveData(m_pimpl);
}

void Client::stop()
{
    m_pimpl->socket.close();
    m_pimpl->unicastSocket.close();
}

void Client::receiveData(const std::shared_ptr<Pimpl>& pimpl)
{
    pimpl->socket.async_receive_from(buffer(pimpl->buffer), pimpl->sender, [pimpl] (const std::error_code& error, size_t bytesReceived) {
        if (error)
        {
            pimpl->parser.reset();
            
            if (pimpl->socket.is_open())
            {
                receiveData(pimpl);
            }
            
            return;
        }

        try
        {
            if (bytesReceived > 0)
            {
                //log::info("SSDP client msg received: {}", std::string_view(pimpl->buffer.data(), bytesReceived));
                pimpl->parser.parse(pimpl->buffer.data(), bytesReceived);
            }
        }
        catch (std::exception& e)
        {
            log::warn("Error parsing http notification: {}", e.what());
        }

        pimpl->parser.reset();
        receiveData(pimpl);
    });
}

void Client::setSearchTimeout(std::chrono::seconds timeout)
{
    m_pimpl->searchTimeout = static_cast<uint32_t>(timeout.count());
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
                                                         "\r\n", s_ssdpIp, s_ssdpPort, m_pimpl->searchTimeout, serviceType));

    sendMessages(m_pimpl->socket, s_ssdpAddressIpv4, req);
}

void Client::search(const char* serviceType, const char* deviceIp)
{
    auto addr = m_pimpl->socket.local_endpoint();
    auto req = std::make_shared<std::string>(fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                                         "HOST:{}:{}\r\n"
                                                         "MAN:\"ssdp:discover\"\r\n"
                                                         "ST:{}\r\n"
                                                         "\r\n", "192.168.1.10", addr.port(), serviceType));

    sendMessages(m_pimpl->unicastSocket, ip::udp::endpoint(ip::address_v4::from_string(deviceIp), s_ssdpPort), req);
}

void Client::sendMessages(asio::ip::udp::socket& sock, const asio::ip::udp::endpoint& addr, std::shared_ptr<std::string> content)
{
    for (uint32_t i = 0; i < s_broadcastRepeatCount; ++i)
    {
        sock.async_send_to(buffer(*content), addr, [content] (const std::error_code& error, size_t) {
            if (error)
            {
                log::warn("Ssdp search failed: {}", error.message());
            }
        });
    }
}

void Client::setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb)
{
    m_pimpl->parser.setHeaderParsedCallback(std::move(cb));
}

}
}
