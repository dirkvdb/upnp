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

#include "utils/format.h"

#include <array>
#include <tuple>
#include <algorithm>
#include <type_traits>
#include <string_view>

namespace upnp
{

template <typename EnumType>
constexpr auto enum_value(EnumType e)
{
    return static_cast<typename std::underlying_type_t<EnumType>>(e);
}

template<typename EnumType>
using IfEndsWithCount = typename std::enable_if_t<std::is_integral<decltype(enum_value(EnumType::EnumCount))>::value, EnumType>;

template<typename EnumType>
using IfEndsWithUnknown = typename std::enable_if_t<std::is_integral<decltype(enum_value(EnumType::Unknown))>::value, EnumType>;

template <typename EnumType>
constexpr std::underlying_type_t<IfEndsWithCount<EnumType>> enum_count()
{
    return enum_value(EnumType::EnumCount);
}

template <typename EnumType>
constexpr std::underlying_type_t<IfEndsWithUnknown<EnumType>> enum_count()
{
    return enum_value(EnumType::Unknown);
}

template <typename EnumType, typename ValueType=const char*>
using EnumMap = std::array<std::tuple<ValueType, EnumType>, enum_count<EnumType>()>;

template <typename EnumType, EnumType endElem, typename ValueType=const char*>
using EnumMapEndsWith = std::array<std::tuple<ValueType, EnumType>, enum_value<EnumType>(endElem) + 1>;

template<typename EnumType, typename ValueType=const char*>
constexpr const EnumMap<EnumType, ValueType>& lut()
{
    static_assert(!std::is_enum<EnumType>::value, "No lookup table provided for type");
    return nullptr;
}

namespace details
{

template <typename EnumType>
EnumType enum_cast(const std::string_view& span)
{
    auto& l = lut<EnumType>();
    auto iter = std::find_if(l.begin(), l.end(), [&] (auto& entry) {
        return strncmp(std::get<0>(entry), span.data(), span.size()) == 0;
    });

    return iter == l.end() ? static_cast<EnumType>(l.size()) : std::get<1>(*iter);
}

}

template <typename EnumType>
IfEndsWithCount<EnumType> enum_cast(const std::string_view& span)
{
    auto value = details::enum_cast<EnumType>(span);
    if (value == EnumType::EnumCount)
    {
        throw std::invalid_argument(fmt::format("Unknown {} enum value: {}", typeid(EnumType).name(), span));
    }

    return value;
}

template <typename EnumType>
IfEndsWithUnknown<EnumType> enum_cast(const std::string_view& span)
{
    return details::enum_cast<EnumType>(span);
}

template <typename EnumType>
constexpr const char* enum_string(EnumType value) noexcept
{
    return std::get<0>(lut<EnumType>()[enum_value(value)]);
}

template <typename EnumValue, typename EnumType>
constexpr EnumValue enum_typecast(EnumType value) noexcept
{
    return std::get<0>(lut<EnumType, EnumValue>()[enum_value(value)]);
}

template <typename EnumType, typename ValueType=const char*>
constexpr bool enumCorrectNess()
{
    for (size_t i = 0; i < lut<EnumType, ValueType>().size(); ++i)
    {
        // Check that the enums appear in the correct order
        if (std::get<1>(lut<EnumType, ValueType>().at(i)) != static_cast<EnumType>(i))
        {
            return false;
        }
    }

    return true;
}

#define ADD_ENUM_MAP(enumType, lutVar) \
    template<> constexpr const EnumMap<enumType>& lut<enumType>() { return lutVar; } \
    static_assert(enumCorrectNess<enumType>(), #enumType " enum converion not correctly ordered or missing entries");

#define ADD_ENUM_MAP_TYPED(enumType, enumValue, lutVar) \
    template<> constexpr const EnumMap<enumType, enumValue>& lut<enumType, enumValue>() { return lutVar; } \
    static_assert(enumCorrectNess<enumType, enumValue>(), #enumType " typed enum converion not correctly ordered or missing entries");

}
