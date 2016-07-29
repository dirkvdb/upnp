#include "upnp/upnp.ssdp.server.h"
#include "upnp.ssdp.parseutils.h"


#include "utils/log.h"
#include "utils/stringoperations.h"

#include <random>

namespace upnp
{
namespace ssdp
{

using namespace utils;
using namespace asio;
using namespace std::chrono_literals;
using namespace std::placeholders;

static const auto s_ssdpIp = ip::address_v4::from_string("239.255.255.250");
static const std::string s_anyAddress = "0.0.0.0";
static const int16_t s_ssdpPort = 1900;
static const uint32_t s_broadcastRepeatCount = 3;
static const auto s_ssdpAddressIpv4 = ip::udp::endpoint(s_ssdpIp, s_ssdpPort);

static const std::string s_aliveNotification =
"NOTIFY * HTTP/1.1\r\n"
"HOST:239.255.255.250:1900\r\n"
"CACHE-CONTROL:max-age=1800\r\n"
"SERVER: UPnP/1.1 1.0\r\n"
"LOCATION:{0}\r\n"
"NT:{1}\r\n"
"NTS:ssdp:alive\r\n"
"USN:{2}\r\n"
"BOOTID.UPNP.ORG: 1\r\n"
"CONFIGID.UPNP.ORG: 1\r\n"
"\r\n";

static const std::string s_byebyeNotification =
"NOTIFY * HTTP/1.1\r\n"
"HOST:239.255.255.250:1900\r\n"
"NT:{0}\r\n"
"NTS:ssdp:byebye\r\n"
"USN:{1}\r\n"
"BOOTID.UPNP.ORG: 1\r\n"
"CONFIGID.UPNP.ORG: 1\r\n"
"\r\n";

static const std::string s_searchResponse =
"HTTP/1.1 200 OK\r\n"
"CACHE-CONTROL:max-age=1800\r\n"
"EXT:\r\n"
"LOCATION:{}\r\n"
"SERVER:UPnP/1.1 1.0\r\n"
"ST:{}\r\n"
"USN:{}\r\n"
"BOOTID.UPNP.ORG:1\r\n"
"CONFIGID.UPNP.ORG:1\r\n"
"\r\n";

namespace
{
    std::string alive(const std::string& location, const std::string& nt, const std::string& nts)
    {
        return fmt::format(s_aliveNotification, location, nt, nts);
    }

    auto s_emptyHandler = [] (const std::error_code&, size_t) {};
}

Server::Server(asio::io_service& io)
: m_io(io)
, m_timer(io)
, m_announceTimer(io)
, m_socket(io, ip::udp::v4())
, m_unicastSocket(io)
{
}

Server::~Server() noexcept = default;

void Server::run(const Device& info, std::chrono::seconds announceInterval)
{
    m_device = info;
    m_announceInterval = announceInterval;

    m_unicastSocket.open(ip::udp::v4());
    m_unicastSocket.set_option(ip::multicast::hops(2));
    m_unicastSocket.set_option(ip::multicast::enable_loopback(true));

    auto addr = ip::udp::endpoint(ip::address_v4::from_string(s_anyAddress), s_ssdpPort);
    m_socket.set_option(ip::udp::socket::reuse_address(true));
    m_socket.bind(addr);
    m_socket.set_option(ip::udp::socket::broadcast(true));

    // join the multicast channel
    m_socket.set_option(ip::multicast::join_group(s_ssdpIp));
    m_socket.set_option(ip::multicast::hops(2));

    m_announceMessages.clear();
    // Three discovery messages for the root device
    m_announceMessages.emplace_back(alive(info.location, "upnp:rootdevice", fmt::format("{}::{}", info.udn, "upnp:rootdevice")));
    m_announceMessages.emplace_back(alive(info.location, info.udn, info.udn));
    m_announceMessages.emplace_back(alive(info.location, deviceTypeToString(info.type), fmt::format("{}::{}", info.udn, deviceTypeToString(info.type))));

    // Once for each service type in each device
    for (auto& svc : info.services)
    {
        m_announceMessages.emplace_back(alive(info.location, serviceTypeToUrnTypeString(svc.second.type),
                                              fmt::format("{}::{}", info.udn, serviceTypeToUrnTypeString(svc.second.type))));
    }

    m_byebyeMessages.clear();
    m_byebyeMessages.emplace_back(fmt::format(s_byebyeNotification, "upnp:rootdevice", info.udn));
    m_byebyeMessages.emplace_back(fmt::format(s_byebyeNotification, deviceTypeToString(info.type), info.udn));

    for (auto& svc : info.services)
    {
        m_byebyeMessages.emplace_back(fmt::format(s_byebyeNotification, serviceTypeToUrnTypeString(svc.second.type), info.udn));
    }

    receiveData();
    announceDevice();
}

void Server::stop(std::function<void()> cb)
{
    m_announceTimer.cancel();
    m_unicastSocket.close();
    announceDeviceStop([this, cb] (const asio::error_code&, size_t) {
        m_socket.close();
        cb();
    });
}

void Server::receiveData()
{
    m_socket.async_receive_from(buffer(m_buffer), m_sender, [this] (const std::error_code& error, size_t bytesReceived) {
        if (error and error != asio::error::message_size)
        {
            log::warn("Ssdp Server: Failed to read from udp socket: {}", error.message());
            receiveData();
            return;
        }

        //log::info("Ssdp recv: {}", std::string(m_buffer.data(), bytesReceived));

        try
        {
            if (bytesReceived > 0)
            {
                SearchParser parser;
                parser.setSearchRequestCallback([this] (auto& host, auto& st, auto delay, auto& addr) { this->respondToSearch(host, st, delay, addr); });
                auto parsed = parser.parse(std::string_view(m_buffer.data(), bytesReceived), m_sender);
                assert(parsed == bytesReceived);
                if (parsed != bytesReceived)
                {
                    log::warn("Ssdp Server: not enough bytes parsed");
                }
            }
        }
        catch (std::exception& e)
        {
            log::warn("Error parsing ssdp server http notification: {}\n{}", e.what(), std::string(m_buffer.data(), bytesReceived));
        }

        receiveData();
    });
}

void Server::sendMessages(const std::vector<std::string>& msgs, std::shared_ptr<asio::steady_timer> timer, int32_t count)
{
    m_socket.async_send_to(buffer(msgs.front()), s_ssdpAddressIpv4, [this, timer, count, &msgs] (const std::error_code& error, size_t) {
        if (error)
        {
            log::warn("Failed to send broadcast message: {}", error.message());
        }

        if ((count - 1) > 0)
        {
            static std::default_random_engine re;
            static std::uniform_int_distribution<> dis(50, 200);

            m_timer.expires_from_now(std::chrono::milliseconds(dis(re)));
            m_timer.async_wait([this, timer, count, &msgs] (const std::error_code& error) {
                if (error != asio::error::operation_aborted)
                {
                    sendMessages(msgs, timer, count - 1);
                }
            });
        }
    });

    for (size_t i = 1; i < msgs.size(); ++i)
    {
        m_socket.async_send_to(buffer(msgs[i]), s_ssdpAddressIpv4, s_emptyHandler);
    }
}

void Server::announceDevice()
{
    sendMessages(m_announceMessages, std::make_shared<asio::steady_timer>(m_io), s_broadcastRepeatCount);
}

void Server::announceDeviceStop(std::function<void(const asio::error_code&, size_t)> cb)
{
    for (size_t i = 1; i < m_byebyeMessages.size() - 1; ++i)
    {
        m_socket.async_send_to(buffer(m_byebyeMessages[i]), s_ssdpAddressIpv4, s_emptyHandler);
    }

    m_socket.async_send_to(buffer(m_byebyeMessages.back()), s_ssdpAddressIpv4, cb);
}

void Server::respondToSearch(const std::string& /*host*/, const std::string& searchTarget, std::chrono::seconds delay, const ip::udp::endpoint& addr)
{
    try
    {
        //log::info("Search request: {} {} {}", host, searchTarget, delay.count());

        if (isResponseNeeded(searchTarget))
        {
            auto response = std::make_shared<std::string>(fmt::format(s_searchResponse, m_device.location, searchTarget, m_device.udn));

            if (delay.count() > 0)
            {
                auto timer = std::make_shared<asio::steady_timer>(m_io);
                static std::default_random_engine re;
                std::uniform_int_distribution<> dis(0, std::chrono::duration_cast<std::chrono::milliseconds>(delay).count());
                timer->expires_from_now(std::chrono::milliseconds(dis(re)));
                timer->async_wait([this, timer, response, addr] (const std::error_code& e) {
                    if (e != asio::error::operation_aborted)
                    {
                        sendResponse(response, timer, addr);
                    }
                });
            }
            else
            {
                sendResponse(response, nullptr, addr);
            }
        }
    }
    catch (const std::exception& e)
    {
        log::warn("Failed to interpret ssdp search request: {}", e.what());
    }
}

void Server::sendResponse(std::shared_ptr<std::string> response, std::shared_ptr<asio::steady_timer> timer, const asio::ip::udp::endpoint& addr)
{
    m_unicastSocket.async_send_to(buffer(*response), addr, [response] (const std::error_code& error, size_t) {
        if (error)
        {
            log::warn("Failed to send search response: {}", error.message());
        }
    });

    timer.reset();
}

bool Server::isResponseNeeded(const std::string& searchTarget)
{
    if (searchTarget == "ssdp:all" ||
        searchTarget == "upnp:rootdevice" ||
        m_device.udn == searchTarget)
    {
        return true;
    }

    auto deviceType = deviceTypeFromString(searchTarget);
    if (deviceType.type != DeviceType::Unknown)
    {
        return (m_device.type.type == deviceType.type && m_device.type.version >= deviceType.version);
    }

    auto serviceType = serviceTypeUrnStringToService(searchTarget);
    auto iter = m_device.services.find(serviceType.type);
    if (iter != m_device.services.end())
    {
        return (iter->second.type.type == serviceType.type && iter->second.type.version >= serviceType.version);
    }

    return false;
}

}
}
