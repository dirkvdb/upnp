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

static const auto s_ssdpIp = ip::address_v4::from_string("239.255.255.250");
static const int16_t s_ssdpPort = 1900;
static const uint32_t s_broadcastRepeatCount = 5;
static const auto s_ssdpAddressIpv4 = ip::udp::endpoint(s_ssdpIp, s_ssdpPort);

Client::Client(io_service& svc)
: m_searchTimeout(3)
, m_service(svc)
, m_socket(svc)
, m_parser(std::make_unique<Parser>())
{
}

Client::~Client() noexcept = default;

void Client::run()
{
    run(ip::udp::endpoint(ip::udp::v4(), 0));
}

void Client::run(const std::string& address)
{
    run(ip::udp::endpoint(ip::address_v4::from_string(address), 0));
}

void Client::run(const asio::ip::udp::endpoint& addr)
{
    m_socket.set_option(ip::udp::socket::reuse_address(true));
    m_socket.set_option(ip::udp::socket::broadcast(true));
    m_socket.open(addr.protocol());
    m_socket.bind(addr);

    // join the multicast channel
    m_socket.set_option(ip::multicast::join_group(s_ssdpIp));
    m_socket.set_option(ip::multicast::hops(4));

    m_socket.async_receive_from(buffer(m_buffer), m_sender, [=] (const std::error_code& error, size_t bytesReceived) {
        if (error)
        {
            m_parser->reset();
            return;
        }

        try
        {
            if (bytesReceived > 0)
            {
                auto parsed = m_parser->parse(m_buffer.data(), bytesReceived);
                assert(parsed == bytesReceived);
            }
            else
            {
                m_parser->reset();
            }
        }
        catch (std::exception& e)
        {
            log::warn("Error parsing http notification: {}", e.what());
            log::info(std::string(m_buffer.data(), bytesReceived));
            m_parser->reset();
        }
    });
}

void Client::stop()
{
    m_socket.close();
}

void Client::setSearchTimeout(std::chrono::seconds timeout)
{
    m_searchTimeout = static_cast<uint32_t>(timeout.count());
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
                                                         "\r\n", s_ssdpIp, s_ssdpPort, m_searchTimeout, serviceType));

    sendMessages(s_ssdpAddressIpv4, req);
}

void Client::search(const char* serviceType, const char* deviceIp)
{
    auto addr = m_socket.local_endpoint();
    auto req = std::make_shared<std::string>(fmt::format("M-SEARCH * HTTP/1.1\r\n"
                                                         "HOST:{}:{}\r\n"
                                                         "MAN:\"ssdp:discover\"\r\n"
                                                         "ST:{}\r\n"
                                                         "\r\n", addr.address(), addr.port(), serviceType));

    sendMessages(ip::udp::endpoint(ip::address_v4::from_string(deviceIp), s_ssdpPort), req);
}

void Client::sendMessages(const asio::ip::udp::endpoint& addr, std::shared_ptr<std::string> content)
{
    for (uint32_t i = 0; i < s_broadcastRepeatCount; ++i)
    {
        m_socket.async_send_to(buffer(*content), addr, [content] (const std::error_code& error, size_t) {
            if (!error)
            {
                log::warn("Ssdp search failed: {}", error.message());
            }
        });
    }
}

void Client::setDeviceNotificationCallback(std::function<void(const DeviceNotificationInfo&)> cb)
{
    m_parser->setHeaderParsedCallback(std::move(cb));
}

}
}
