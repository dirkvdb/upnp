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
    Parser parser;
    std::array<char, 1024> buffer;
};

struct Client::Pimpl
{
    Pimpl(io_service& io)
    : searchTimeout(3)
    , service(io)
    , unicast(std::make_unique<Receiver>(io))
    , multicast(std::make_unique<Receiver>(io))
    {
    }

    uint32_t searchTimeout;
    io_service& service;
    ip::udp::endpoint sender;
    
    std::shared_ptr<Receiver> unicast;
    std::shared_ptr<Receiver> multicast;
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

void Client::run(const ip::udp::endpoint& addr)
{
    m_pimpl->unicast->socket.open(addr.protocol());
    m_pimpl->unicast->socket.set_option(ip::multicast::enable_loopback(true));
    m_pimpl->unicast->socket.set_option(ip::multicast::hops(4));
    m_pimpl->unicast->socket.bind(ip::udp::endpoint(addr.address(), 0));

    m_pimpl->multicast->socket.open(addr.protocol());
    m_pimpl->multicast->socket.set_option(ip::udp::socket::reuse_address(true));
    m_pimpl->multicast->socket.set_option(ip::udp::socket::broadcast(true));
    m_pimpl->multicast->socket.set_option(ip::multicast::enable_loopback(true));
    m_pimpl->multicast->socket.set_option(ip::multicast::hops(4));
    m_pimpl->multicast->socket.bind(addr);
    m_pimpl->multicast->socket.set_option(ip::multicast::join_group(s_ssdpIp));

    receiveData(m_pimpl->sender, m_pimpl->unicast);
    receiveData(m_pimpl->sender, m_pimpl->multicast);
}

void Client::stop()
{
    m_pimpl->multicast->socket.close();
    m_pimpl->unicast->socket.close();
}

void Client::receiveData(ip::udp::endpoint sender, const std::shared_ptr<Receiver>& receiver)
{
    receiver->socket.async_receive_from(buffer(receiver->buffer), sender, [sender, receiver] (const std::error_code& error, size_t bytesReceived) {
        if (error)
        {
            receiver->parser.reset();
            
            if (receiver->socket.is_open())
            {
                receiveData(sender, receiver);
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
        receiveData(sender, receiver);
    });
}

void Client::handleMessage(Receiver& receiver, size_t bytesReceived)
{
    
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

    sendMessages(m_pimpl->unicast->socket, s_ssdpAddressIpv4, req);
}

void Client::search(const char* serviceType, const char* deviceIp)
{
    auto addr = m_pimpl->unicast->socket.local_endpoint();
    auto req = std::make_shared<std::string>(fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                                         "HOST:{}:{}\r\n"
                                                         "MAN:\"ssdp:discover\"\r\n"
                                                         "ST:{}\r\n"
                                                         "\r\n", "192.168.1.10", addr.port(), serviceType));

    sendMessages(m_pimpl->unicast->socket, ip::udp::endpoint(ip::address_v4::from_string(deviceIp), s_ssdpPort), req);
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
    m_pimpl->unicast->parser.setHeaderParsedCallback(cb);
    m_pimpl->multicast->parser.setHeaderParsedCallback(cb);
}

}
}
