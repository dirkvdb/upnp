#include "upnp/upnpssdpclient.h"

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp/upnpssdpclient.h"
#include "upnphttpparser.h"

namespace upnp
{
namespace ssdp
{

inline void parseUSN(const std::string& usn, DeviceDiscoverInfo& info)
{
    try
    {
        std::regex re(R"((uuid:[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})(?:::(\S*))?)");
        std::smatch match;
        if (std::regex_match(usn, match, re))
        {
            info.deviceId = match.str(1);

            if (match.size() > 2)
            {
                info.deviceType = match.str(2);
            }
        }
        else
        {
            throw std::runtime_error("Failed to parse USN");
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse USN: {}", e.what()));
    }
}

inline uint32_t parseCacheControl(const std::string& cacheControl)
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

inline DeviceDiscoverInfo parseNotification(const std::string& msg)
{
    DeviceDiscoverInfo info;
            
    http::Parser parser(http::Type::Request);
    parser.setHeaderCallback([&] (const char* field, size_t fieldLength, const char* value, size_t valueLength) {
        if (strncasecmp(field, "LOCATION", fieldLength) == 0)
        {
            info.location = std::string(value, valueLength);
        }
        else if (strncasecmp(field, "NT", fieldLength) == 0)
        {
            info.deviceType = std::string(value, valueLength);
        }
        else if (strncasecmp(field, "USN", fieldLength) == 0)
        {
            parseUSN(std::string(value, valueLength), info);
        }
        else if (strncasecmp(field, "CACHE-CONTROL", fieldLength) == 0)
        {
            info.expirationTime = parseCacheControl(std::string(value, valueLength));
        }
    });

    parser.parse(msg);
    return info;
}

}
}
