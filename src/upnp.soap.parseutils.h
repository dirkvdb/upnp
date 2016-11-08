#pragma once

#include <regex>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "rapidxml.hpp"
#include "upnp.soap.client.h"

namespace upnp
{
namespace soap
{

inline std::chrono::seconds parseTimeout(std::string_view timeout)
{
    if (timeout == "Second-infinite")
    {
        return std::chrono::seconds(0);
    }

    try
    {
        std::regex re(R"(Second-(\d+))");
        std::smatch match;
        std::string input = timeout.to_string();
        if (std::regex_match(input, match, re))
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

inline std::tuple<std::string, std::string> parseAction(const std::string& action)
{
    try
    {
        std::regex re(R"(\"(.*)#(.*)\")");
        std::smatch match;
        if (std::regex_match(action, match, re))
        {
            return std::make_tuple(match.str(1), match.str(2));
        }
        else
        {
            throw std::runtime_error("Failed to parse soap action: " + action);
        }
    }
    catch (const std::regex_error& e)
    {
        throw std::runtime_error(std::string("Failed to parse soap action: ") + e.what());
    }
}

inline Fault parseFault(const std::string& xml)
{
    using namespace rapidxml_ns;

    xml_document<char> doc;
    doc.parse<parse_non_destructive | parse_trim_whitespace>(xml.c_str());

    auto& detailNode = doc.first_node_ref().first_node_ref().first_node_ref().first_node_ref("detail");
    auto& upnpErrorNode = detailNode.first_node_ref("UPnPError");

    // Required error code
    auto ec = std::atoi(upnpErrorNode.first_node_ref("errorCode").value_string().c_str());

    // Optional error description
    auto* descriptionNode = upnpErrorNode.first_node("errorDescription");
    if (descriptionNode)
    {
        return Fault(ec, descriptionNode->value_string());
    }

    return Fault(ec);
}

}
}
