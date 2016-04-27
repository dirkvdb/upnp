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
#include "upnp.enumutils.h"

namespace upnp
{

using namespace ContentDirectory;

constexpr std::tuple<const char*, Action> s_actionNames[] {
    { "GetSearchCapabilities",  Action::GetSearchCapabilities },
    { "GetSortCapabilities",    Action::GetSortCapabilities },
    { "GetSystemUpdateID",      Action::GetSystemUpdateID },
    { "Browse",                 Action::Browse },
    { "Search",                 Action::Search }
};

constexpr std::tuple<const char*, Variable> s_variableNames[] {
    { "ContainerUpdateIDs",         Variable::ContainerUpdateIDs },
    { "TransferIDs",                Variable::TransferIDs },
    { "SystemUpdateID",             Variable::SystemUpdateID },
    { "A_ARG_TYPE_ObjectID",        Variable::ArgumentTypeObjectID },
    { "A_ARG_TYPE_Result",          Variable::ArgumentTypeResult },
    { "A_ARG_TYPE_SearchCriteria",  Variable::ArgumentTypeSearchCriteria },
    { "A_ARG_TYPE_Flag",            Variable::ArgumentTypeBrowseFlag },
    { "A_ARG_TYPE_Filter",          Variable::ArgumentTypeFilter },
    { "A_ARG_TYPE_SortCriteria",    Variable::ArgumentTypeSortCriteria },
    { "A_ARG_TYPE_Index",           Variable::ArgumentTypeIndex },
    { "A_ARG_TYPE_Count",           Variable::ArgumentTypeCount },
    { "A_ARG_TYPE_UpdateID",        Variable::ArgumentTypeUpdateID },
    { "SearchCapabilities",         Variable::SearchCapabilities },
    { "SortCapabilities",           Variable::SortCapabilities }
};

constexpr std::tuple<const char*, BrowseFlag> s_browseFlagNames[] {
    { "BrowseMetadata", BrowseFlag::Metadata },
    { "BrowseDirectChildren", BrowseFlag::DirectChildren }
};

template<> constexpr const std::tuple<const char*, Action>* lut<Action>() { return s_actionNames; }
template<> constexpr const std::tuple<const char*, Variable>* lut<Variable>() { return s_variableNames; }
template<> constexpr const std::tuple<const char*, BrowseFlag>* lut<BrowseFlag>() { return s_browseFlagNames; }

static_assert(enumCorrectNess<Action>(), "Action enum converion not correctly ordered or missing entries");
static_assert(enumCorrectNess<Variable>(), "Action enum converion not correctly ordered or missing entries");
static_assert(enumCorrectNess<BrowseFlag>(), "BrowseFlag enum converion not correctly ordered or missing entries");

Action ContentDirectory::actionFromString(const char* data, size_t size)
{
    return fromString<Action>(data, size);
}

Action ContentDirectory::actionFromString(const std::string& value)
{
    return actionFromString(value.c_str(), value.size());
}

const char* ContentDirectory::actionToString(Action value) noexcept
{
    return toString(value);
}

Variable ContentDirectory::variableFromString(const char* data, size_t size)
{
    return fromString<Variable>(data, size);
}

Variable ContentDirectory::variableFromString(const std::string& value)
{
    return variableFromString(value.c_str(), value.size());
}

const char* ContentDirectory::variableToString(Variable value) noexcept
{
    return toString(value);
}

BrowseFlag browseFlagFromString(const std::string& value)
{
    return fromString<BrowseFlag>(value.c_str(), value.size());
}

std::string browseFlagToString(BrowseFlag value) noexcept
{
    return toString(value);
}

SortType sortTypeFromString(char c)
{
    if (c == '-')   return SortType::Descending;
    if (c == '+')   return SortType::Ascending;

    throw Exception("Invalid sort character: {}", c);
}

}
