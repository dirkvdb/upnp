#pragma once

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "rapidxml.hpp"

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

struct Fault
{
    uint32_t errorCode;
    std::string errorDescription;
};

inline Fault parseFault(const std::string& xml)
{
    using namespace rapidxml_ns;

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());

    Fault fault;
    auto& detailNode = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("detail");
    auto& upnpErrorNode = detailNode.first_node_ref("UPnPError");

    // Required error code
    fault.errorCode = std::atoi(upnpErrorNode.first_node_ref("errorCode").value_string().c_str());

    // Optional error description
    auto* descriptionNode = upnpErrorNode.first_node("errorDescription");
    if (descriptionNode)
    {
        fault.errorDescription = descriptionNode->value_string();
    }

    return fault;
}

}
}
