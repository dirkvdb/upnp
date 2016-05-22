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

static const char* s_ssdpIp = "239.255.255.250";
static const std::string s_anyAddress = "0.0.0.0";
static const int32_t s_ssdpPort = 1900;
static const uint32_t s_broadcastRepeatCount = 3;
static const uv::Address s_ssdpAddressIpv4 = uv::Address::createIp4(s_ssdpIp, s_ssdpPort);

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
}

Server::Server(uv::Loop& loop)
: m_loop(loop)
, m_timer(loop)
, m_socket(loop)
, m_responseSocket(loop)
, m_parser(std::make_unique<SearchParser>())
{
    m_parser->setSearchRequestCallback([this] (auto& host, auto& st, auto delay) { this->respondToSearch(host, st, delay); });
}

Server::~Server() noexcept = default;

void Server::run(const Device& info)
{
    m_device = info;

    // join the multicast channel
    m_socket.bind(uv::Address::createIp4(s_anyAddress, s_ssdpPort));
    m_socket.setBroadcast(true);
    m_socket.setMulticastTtl(2);
    m_socket.setMemberShip(s_ssdpIp, uv::socket::Udp::MemberShip::JoinGroup);

    m_socket.recv([=] (const std::string& msg) {
        try
        {
            if (!msg.empty())
            {
                auto parsed = m_parser->parse(msg);
                assert(parsed == msg.size());
                if (parsed != msg.size())
                {
                    log::warn("Ssdp Server: not enough bytes parsed");
                }
            }
            else
            {
                m_parser->reset();
            }
        }
        catch (std::exception& e)
        {
            log::warn("Error parsing ssdp server http notification: {}", e.what());
            m_parser->reset();
        }
    });

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

    announceDevice();
}

void Server::stop(std::function<void()> cb)
{
    m_responseSocket.close(nullptr);
    announceDeviceStop([this, cb] (int32_t) {
        m_socket.close(cb);
    });
}

void Server::sendMessages(const std::vector<std::string>& msgs, std::shared_ptr<uv::Timer> timer, int32_t count)
{
    m_socket.send(s_ssdpAddressIpv4, msgs.front(), [this, timer, count, &msgs] (int32_t status) {
        if (status < 0)
        {
            log::warn("Failed to send broadcast message: {}", uv::getErrorString(status));
        }

        if ((count - 1) > 0)
        {
            static std::default_random_engine re;
            static std::uniform_int_distribution<> dis(50, 200);

            m_timer.start(std::chrono::milliseconds(dis(re)), [this, timer, count, &msgs] () {
                sendMessages(msgs, timer, count - 1);
            });
        }
    });

    for (size_t i = 1; i < msgs.size(); ++i)
    {
        m_socket.send(s_ssdpAddressIpv4, msgs[i], nullptr);
    }
}

void Server::announceDevice()
{
    sendMessages(m_announceMessages, std::make_shared<uv::Timer>(m_loop), s_broadcastRepeatCount);
}

void Server::announceDeviceStop(std::function<void(int32_t)> cb)
{
    for (size_t i = 1; i < m_byebyeMessages.size() - 1; ++i)
    {
        m_socket.send(s_ssdpAddressIpv4, m_byebyeMessages[i], nullptr);
    }

    m_socket.send(s_ssdpAddressIpv4, m_byebyeMessages.back(), cb);
}

void Server::respondToSearch(const std::string& host, const std::string& searchTarget, std::chrono::seconds delay)
{
    try
    {
        //log::info("Search request: {} {} {}", host, searchTarget, delay.count());

        auto addr = uv::Address::createIp4FromHost(host, s_ssdpPort);

        if (searchTarget == "ssdp:all" ||
            searchTarget == "upnp:rootdevice")
        {
            auto response = std::make_shared<std::string>(fmt::format(s_searchResponse, m_device.location, searchTarget, m_device.udn));
            m_socket.send(addr, *response, [response] (int32_t status) {
                if (status < 0)
                {
                    log::warn("Failed to send search response: {}", uv::getErrorString(status));
                }
            });
        }
    }
    catch (const std::exception& e)
    {
        log::warn("Failed to interpret ssdp search request: {}", e.what());
    }
}

}
}
