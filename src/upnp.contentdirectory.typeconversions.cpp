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

#include "upnp.contentdirectory.typeconversions.h"
#include "upnp.enumutils.h"

namespace upnp
{

using namespace ContentDirectory;

static constexpr EnumMap<Action> s_actionNames  {{
    { "GetSearchCapabilities",  Action::GetSearchCapabilities },
    { "GetSortCapabilities",    Action::GetSortCapabilities },
    { "GetSystemUpdateID",      Action::GetSystemUpdateID },
    { "Browse",                 Action::Browse },
    { "Search",                 Action::Search }
}};

static constexpr EnumMap<Variable> s_variableNames  {{
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
}};

static constexpr EnumMap<BrowseFlag> s_browseFlagNames  {{
    { "BrowseMetadata", BrowseFlag::Metadata },
    { "BrowseDirectChildren", BrowseFlag::DirectChildren }
}};

ADD_ENUM_MAP(Action, s_actionNames)
ADD_ENUM_MAP(Variable, s_variableNames)
ADD_ENUM_MAP(BrowseFlag, s_browseFlagNames)

Action ContentDirectory::actionFromString(gsl::cstring_span<> data)
{
    return enum_cast<Action>(data);
}

const char* ContentDirectory::actionToString(Action value) noexcept
{
    return enum_string(value);
}

Variable ContentDirectory::variableFromString(gsl::cstring_span<> data)
{
    return enum_cast<Variable>(data);
}

const char* ContentDirectory::variableToString(Variable value) noexcept
{
    return enum_string(value);
}

BrowseFlag browseFlagFromString(gsl::cstring_span<> data)
{
    return enum_cast<BrowseFlag>(data);
}

std::string browseFlagToString(BrowseFlag value) noexcept
{
    return enum_string(value);
}

SortType sortTypeFromString(char c)
{
    if (c == '-')   return SortType::Descending;
    if (c == '+')   return SortType::Ascending;

    throw Exception("Invalid sort character: {}", c);
}

}
