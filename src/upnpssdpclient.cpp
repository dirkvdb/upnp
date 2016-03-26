#include "upnp/upnpssdpclient.h"

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"

namespace upnp
{
namespace ssdp
{

static const std::string g_ssdpIp = "239.255.255.250";
static const std::string g_anyAddress = "0.0.0.0";
static const int32_t g_ssdpPort = 1900;
static const uint32_t g_broadcastRepeatCount = 5;
static const uv::Address g_ssdpAddressIpv4 = uv::Address::createIp4(g_ssdpIp, g_ssdpPort);

namespace
{

//constexpr int length(const char* str)
//{
//    return *str ? 1 + length(str + 1) : 0;
//}

std::vector<std::pair<const char*, uint32_t>> tokenize(const std::string& str, const std::string& delimiter)
{
    std::vector<std::pair<const char*, uint32_t>> tokens;
    size_t pos = 0;
    size_t index = 0;

    while ((pos = str.find(delimiter, index)) != std::string::npos)
    {
        tokens.emplace_back(&str[index], pos - index);
        index = pos + delimiter.size();
    }

    if (index < str.size())
    {
        tokens.emplace_back(&str[index], str.size() - index);
    }

    return tokens;
}

void parseUSN(const std::string& usn, DeviceDiscoverInfo& info)
{
    try
    {
        std::regex re(R"((uuid:\S{36})(?:::(\S*))?)");
        std::smatch match;
        if (std::regex_match(usn, match, re))
        {
            info.deviceId = match.str(1);

            if (match.size() > 2)
            {
                info.deviceType = match.str(2);
            }
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse USN: {}", e.what()));
    }
}

uint32_t parseCacheControl(const std::string& cacheControl)
{
    try
    {
        std::regex re(R"(max-age=(\d+)))");
        std::smatch match;
        if (std::regex_match(cacheControl, match, re))
        {
            return std::stoi(match.str(1));
        }

        throw std::runtime_error("Failed to parse Cache Control");
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse Cache Control: {}", e.what()));
    }
}

DeviceDiscoverInfo parseNotification(const std::string& message)
{
    DeviceDiscoverInfo info;
    auto tokens = tokenize(message, "\r\n");

    for (auto& token : tokens)
    {
        const auto line = std::string(token.first, token.second);
        auto pos = line.find_first_of(':');
        if (pos == std::string::npos)
        {
            continue;
        }

        if (utils::stringops::startsWith(line, "LOCATION:"))
        {
            info.location = utils::stringops::trim(&line[pos + 1]);
        }
        if (utils::stringops::startsWith(line, "NT:"))
        {
            info.deviceType = utils::stringops::trim(&line[pos + 1]);
        }
        if (utils::stringops::startsWith(line, "USN:"))
        {
            parseUSN(utils::stringops::trim(utils::stringops::trim(&line[pos + 1])), info);
        }
        if (utils::stringops::startsWith(line, "CACHE-CONTROL:"))
        {
            info.expirationTime = parseCacheControl(utils::stringops::trim(&line[pos + 1]));
        }
    }

    return info;
}

}

Client::Client(uv::Loop& loop)
: m_socket(loop)
{
}

void Client::run()
{
    run(g_anyAddress);
}

void Client::run(const std::string& address)
{
    m_socket.bind(uv::Address::createIp4(address, g_ssdpPort), uv::socket::UdpFlag::ReuseAddress);
    m_socket.setBroadcast(true);

    // join the multicast channel
    m_socket.setMemberShip(g_ssdpIp, uv::socket::Udp::MemberShip::JoinGroup);

    m_socket.recv([=] (const std::string& msg) {
        if (msg.empty())
        {
            utils::log::info("Read done");
            return;
        }

        try
        {
            utils::log::info("Read {}", msg);
            if (m_cb && utils::stringops::startsWith(msg, "NOTIFY * HTTP/1.1\r\n"))
            {
                m_cb(parseNotification(msg.substr(19)));
            }
        }
        catch (std::runtime_error& e)
        {
            utils::log::warn(e.what());
        }
    });
}

void Client::stop()
{
    m_socket.close([] () {
        utils::log::debug("SSDP client socket closed");
    });
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

void Client::setDiscoverCallback(std::function<void(const DeviceDiscoverInfo&)> cb)
{
    m_cb = std::move(cb);
}

}
}
