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

#pragma once

#include <cinttypes>
#include <vector>
#include "upnp/upnpitem.h"

namespace upnp
{
namespace ContentDirectory
{

struct ActionResult
{
    uint32_t totalMatches = 0;
    uint32_t numberReturned = 0;
    uint32_t updateId = 0;
    std::vector<Item> result;
};


enum class Action
{
    GetSearchCapabilities,
    GetSortCapabilities,
    GetSystemUpdateID,
    Browse,
    Search,
    EnumCount
};

enum class Variable
{
    ContainerUpdateIDs,
    TransferIDs,
    SystemUpdateID,
    ArgumentTypeObjectID,
    ArgumentTypeResult,
    ArgumentTypeSearchCriteria,
    ArgumentTypeBrowseFlag,
    ArgumentTypeFilter,
    ArgumentTypeSortCriteria,
    ArgumentTypeIndex,
    ArgumentTypeCount,
    ArgumentTypeUpdateID,
    SearchCapabilities,
    SortCapabilities,
    EnumCount
};

enum class BrowseFlag
{
    Metadata,
    DirectChildren
};

enum class SortType
{
    Ascending,
    Descending
};

struct SortProperty
{
    SortProperty(Property p, SortType t) : prop(p), type(t) {}

    Property prop;
    SortType type;
};

Action actionFromString(const std::string& action);
Action actionFromString(const char* data, size_t dataSize);
const char* actionToString(Action action) noexcept;

Variable variableFromString(const std::string& var);
Variable variableFromString(const char* data, size_t dataSize);
const char* variableToString(Variable var) noexcept;

inline BrowseFlag browseFlagFromString(const std::string& browseFlag)
{
    if (browseFlag == "BrowseMetadata")         return BrowseFlag::Metadata;
    if (browseFlag == "BrowseDirectChildren")   return BrowseFlag::DirectChildren;

    throw Exception("Unknown ContentDirectory browse flag: {}", browseFlag);
}

inline std::string browseFlagToString(BrowseFlag browseFlag)
{
    switch (browseFlag)
    {
        case BrowseFlag::DirectChildren:    return "BrowseDirectChildren";
        case BrowseFlag::Metadata:          return "BrowseMetadata";

        default:
            throw Exception("Unknown ContentDirectory BrowseFlag: {}", static_cast<int32_t>(browseFlag));
    }
}

inline SortType sortTypeFromString(char c)
{
    if (c == '-')   return SortType::Descending;
    if (c == '+')   return SortType::Ascending;

    throw Exception("Invalid sort character: {}", c);
}

}
}
