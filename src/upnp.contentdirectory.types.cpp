//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "upnp/upnp.contentdirectory.types.h"

#include <array>

namespace upnp
{
namespace ContentDirectory
{

template <typename EnumType>
using EnumLookup = std::array<const char*, static_cast<uint32_t>(EnumType::EnumCount)>;

static constexpr EnumLookup<Action> s_actionNames {{
    "GetSearchCapabilities",
    "GetSortCapabilities",
    "GetSystemUpdateID",
    "Browse",
    "Search"
}};

static constexpr EnumLookup<Variable> s_variableNames {{
    "ContainerUpdateIDs",
    "TransferIDs",
    "SystemUpdateID",
    "A_ARG_TYPE_ObjectID",
    "A_ARG_TYPE_Result",
    "A_ARG_TYPE_SearchCriteria",
    "A_ARG_TYPE_Flag",
    "A_ARG_TYPE_Filter",
    "A_ARG_TYPE_SortCriteria",
    "A_ARG_TYPE_Index",
    "A_ARG_TYPE_Count",
    "A_ARG_TYPE_UpdateID",
    "SearchCapabilities",
    "SortCapabilities"
}};

Action actionFromString(const std::string& value)
{
    return actionFromString(value.c_str(), value.size());
}

Action actionFromString(const char* data, size_t dataSize)
{
    for (uint32_t i = 0; i < s_actionNames.size(); ++i)
    {
        if (strncmp(s_actionNames[i], data, dataSize) == 0)
        {
            return static_cast<Action>(i);
        }
    }

    throw Exception("Unknown ContentDirectory action: {}", std::string(data, dataSize));
}

const char* actionToString(Action value) noexcept
{
    assert(static_cast<uint32_t>(value) < s_actionNames.size());
    return s_actionNames[static_cast<int>(value)];
}

Variable variableFromString(const std::string& value)
{
    return variableFromString(value.c_str(), value.size());
}

Variable variableFromString(const char* data, size_t dataSize)
{
    for (uint32_t i = 0; i < s_variableNames.size(); ++i)
    {
        if (strncmp(s_variableNames[i], data, dataSize) == 0)
        {
            return static_cast<Variable>(i);
        }
    }

    throw Exception("Unknown ContentDirectory variable: {}", std::string(data, dataSize));
}

const char* variableToString(Variable value) noexcept
{
    assert(static_cast<uint32_t>(value) < s_variableNames.size());
    return s_variableNames[static_cast<int>(value)];
}

// BrowseFlag browseFlagFromString(const std::string& browseFlag)
// {
//     if (browseFlag == "BrowseMetadata")         return BrowseFlag::Metadata;
//     if (browseFlag == "BrowseDirectChildren")   return BrowseFlag::DirectChildren;

//     throw Exception("Unknown ContentDirectory browse flag: {}", browseFlag);
// }

// std::string browseFlagToString(BrowseFlag browseFlag)
// {
//     switch (browseFlag)
//     {
//         case BrowseFlag::DirectChildren:    return "BrowseDirectChildren";
//         case BrowseFlag::Metadata:          return "BrowseMetadata";

//         default:
//             throw Exception("Unknown ContentDirectory BrowseFlag: {}", static_cast<int32_t>(browseFlag));
//     }
// }

// SortType sortTypeFromString(char c)
// {
//     if (c == '-')   return SortType::Descending;
//     if (c == '+')   return SortType::Ascending;

//     throw Exception("Invalid sort character: {}", c);
// }

}
}
