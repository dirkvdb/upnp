#pragma once

#include "upnp/upnp.ssdp.client.h"

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "upnp.http.parser.h"

namespace upnp
{
namespace ssdp
{

inline void parseUSN(const std::string& usn, DeviceNotificationInfo& info)
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
        std::regex re(R"(max-age=(\d+))");
        std::smatch match;
        if (std::regex_match(cacheControl, match, re))
        {
            return std::stoi(match.str(1));
        }

        throw std::runtime_error("Failed to parse Cache Control");
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse Cache Control: {} ({})", cacheControl, e.what()));
    }
}

inline NotificationType notificationTypeFromString(const char* type, size_t length)
{
    if (strncmp(type, "ssdp:alive", length) == 0)
    {
        return NotificationType::Alive;
    }
    else if (strncmp(type, "ssdp:byebye", length) == 0)
    {
        return NotificationType::ByeBye;
    }

    throw std::runtime_error("Invalid notification type: " + std::string(type, length));
}

inline NotificationType notificationTypeFromString(const std::string& str)
{
    return notificationTypeFromString(str.data(), str.size());
}

inline DeviceNotificationInfo parseNotification(const std::string& msg)
{
    DeviceNotificationInfo info;
    
    http::Parser parser(http::Type::Both);

    parser.setHeadersCompletedCallback([&] () {
        parseUSN(parser.headerValue("USN"), info);
        info.location = parser.headerValue("LOCATION");
        info.expirationTime = parseCacheControl(parser.headerValue("CACHE-CONTROL"));
        
        if (parser.getMethod() == http::Method::Notify)
        {
            // spontaneous notify message
            info.type = notificationTypeFromString(parser.headerValue("NTS"));
            info.deviceType = parser.headerValue("NT");
        }
        else
        {
            assert(parser.getStatus() == 200);
            // response to a search
            // direct responses do not fill in the NTS, mark them as alive
            info.type = NotificationType::Alive;
            info.deviceType = parser.headerValue("ST");
        }
    });
    
    parser.parse(msg);
    assert(!info.deviceType.empty());
    return info;
}

}
}
