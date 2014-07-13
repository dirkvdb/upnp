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

#ifndef UPNP_CONTENT_DIRECTORY_TYPES_H
#define UPNP_CONTENT_DIRECTORY_TYPES_H

namespace upnp
{
namespace ContentDirectory
{

struct ActionResult
{
    uint32_t totalMatches = 0;
    uint32_t numberReturned = 0;
    uint32_t updateId = 0;
    std::vector<ItemPtr> result;
};


enum class Action
{
    GetSearchCapabilities,
    GetSortCapabilities,
    GetSystemUpdateID,
    Browse,
    Search
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
    SortCapabilities
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

inline Action actionFromString(const std::string& action)
{
    if (action == "GetSearchCapabilities")  return Action::GetSearchCapabilities;
    if (action == "GetSortCapabilities")    return Action::GetSortCapabilities;
    if (action == "GetSystemUpdateID")      return Action::GetSystemUpdateID;
    if (action == "Browse")                 return Action::Browse;
    if (action == "Search")                 return Action::Search;
    
    throw std::logic_error("Unknown ContentDirectory action:" + action);
}

inline std::string actionToString(Action action)
{
    switch (action)
    {
        case Action::GetSearchCapabilities:     return "GetSearchCapabilities";
        case Action::GetSortCapabilities:       return "GetSortCapabilities";
        case Action::GetSystemUpdateID:         return "GetSystemUpdateID";
        case Action::Browse:                    return "Browse";
        case Action::Search:                    return "Search";
            
        default:
            throw std::logic_error("Unknown ContentDirectory action");
    }
}

inline Variable variableFromString(const std::string& var)
{
    if (var == "ContainerUpdateIDs")                return Variable::ContainerUpdateIDs;
    if (var == "TransferIDs")                       return Variable::TransferIDs;
    if (var == "SystemUpdateID")                    return Variable::SystemUpdateID;
    if (var == "A_ARG_TYPE_ObjectID")               return Variable::ArgumentTypeObjectID;
    if (var == "A_ARG_TYPE_Result")                 return Variable::ArgumentTypeResult;
    if (var == "A_ARG_TYPE_SearchCriteria")         return Variable::ArgumentTypeSearchCriteria;
    if (var == "A_ARG_TYPE_Flag")                   return Variable::ArgumentTypeBrowseFlag;
    if (var == "A_ARG_TYPE_Filter")                 return Variable::ArgumentTypeFilter;
    if (var == "A_ARG_TYPE_SortCriteria")           return Variable::ArgumentTypeSortCriteria;
    if (var == "A_ARG_TYPE_Index")                  return Variable::ArgumentTypeIndex;
    if (var == "A_ARG_TYPE_Count")                  return Variable::ArgumentTypeCount;
    if (var == "A_ARG_TYPE_UpdateID")               return Variable::ArgumentTypeUpdateID;
    if (var == "SearchCapabilities")                return Variable::SearchCapabilities;
    if (var == "SortCapabilities")                  return Variable::SortCapabilities;

    throw std::logic_error("Unknown ContentDirectory variable:" + var);
}

inline std::string variableToString(Variable var)
{
    switch (var)
    {
        case Variable::ContainerUpdateIDs:                  return "ContainerUpdateIDs";
        case Variable::TransferIDs:                         return "TransferIDs";
        case Variable::SystemUpdateID:                      return "SystemUpdateID";
        case Variable::ArgumentTypeObjectID:                return "A_ARG_TYPE_ObjectID";
        case Variable::ArgumentTypeResult:                  return "A_ARG_TYPE_Result";
        case Variable::ArgumentTypeSearchCriteria:          return "A_ARG_TYPE_SearchCriteria";
        case Variable::ArgumentTypeBrowseFlag:              return "A_ARG_TYPE_Flag";
        case Variable::ArgumentTypeFilter:                  return "A_ARG_TYPE_Filter";
        case Variable::ArgumentTypeSortCriteria:            return "A_ARG_TYPE_SortCriteria";
        case Variable::ArgumentTypeIndex:                   return "A_ARG_TYPE_Index";
        case Variable::ArgumentTypeCount:                   return "A_ARG_TYPE_Count";
        case Variable::ArgumentTypeUpdateID:                return "A_ARG_TYPE_UpdateID";
        case Variable::SearchCapabilities:                  return "SearchCapabilities";
        case Variable::SortCapabilities:                    return "SortCapabilities";
        
        default:
            throw std::logic_error("Unknown ContentDirectory variable");
    }
}

inline BrowseFlag browseFlagFromString(const std::string& browseFlag)
{
    if (browseFlag == "BrowseMetadata")         return BrowseFlag::Metadata;
    if (browseFlag == "BrowseDirectChildren")   return BrowseFlag::DirectChildren;
    
    throw std::logic_error("Unknown ContentDirectory browse flag:" + browseFlag);
}

inline std::string browseFlagToString(BrowseFlag browseFlag)
{
    switch (browseFlag)
    {
        case BrowseFlag::DirectChildren:    return "BrowseDirectChildren";
        case BrowseFlag::Metadata:          return "BrowseMetadata";
            
        default:
            throw std::logic_error("Unknown ContentDirectory BrowseFlag");
    }
}

inline SortType sortTypeFromString(char c)
{
    if (c == '-')   return SortType::Descending;
    if (c == '+')   return SortType::Ascending;

    throw std::logic_error("Invalid sort character: " + std::string(1, c));
}

}
}

#endif
