#pragma once

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"

namespace upnp
{
namespace soap
{

inline std::chrono::seconds parseTimeout(const std::string& timeout)
{
    if (timeout == "Second-infinite")
    {
        return std::chrono::seconds(0);
    }

    try
    {
        std::regex re(R"(Second-(\d+))");
        std::smatch match;
        if (std::regex_match(timeout, match, re))
        {
            return std::chrono::seconds(std::stoi(match.str(1)));
        }
        else
        {
            throw std::runtime_error("Failed to parse Timeout header");
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(fmt::format("Failed to parse Timeout header: {}", e.what()));
    }
}

}
}
