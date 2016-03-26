#include "upnp/upnpssdpclient.h"

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp/upnpssdpclient.h"

namespace upnp
{
namespace ssdp
{

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
}
