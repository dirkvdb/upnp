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

#include <array>
#include <tuple>
#include <type_traits>

#include "upnp/upnptypes.h"

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

template <typename EnumType>
using EnumMap = std::array<std::tuple<const char*, EnumType>, enum_count<EnumType>()>;

template <typename EnumType, EnumType endElem>
using EnumMapEndsWith = std::array<std::tuple<const char*, EnumType>, enum_value<EnumType>(endElem) + 1>;

template<typename EnumType>
constexpr const EnumMap<EnumType>& lut()
{
    static_assert(!std::is_enum<EnumType>::value, "No lookup table provided for type");
    return nullptr;
}

namespace details
{

template <typename EnumType, typename Cb>
constexpr EnumType enum_cast(Cb&& cb)
{
    auto& l = lut<EnumType>();
    auto iter = std::find_if(l.begin(), l.end(), [&] (auto& entry) {
        return cb(std::get<0>(entry));
    });

    return iter == l.end() ? static_cast<EnumType>(l.size()) : std::get<1>(*iter);
}

template <typename EnumType>
constexpr EnumType enum_cast(const char* data)
{
    return enum_cast<EnumType>([=] (const char* value) {
        return strcmp(value, data) == 0;
    });
}

template <typename EnumType>
constexpr EnumType enum_cast(const char* data, size_t dataSize)
{
    return enum_cast<EnumType>([=] (const char* value) {
        return strncmp(value, data, dataSize) == 0;
    });
}

}

template <typename EnumType>
constexpr IfEndsWithCount<EnumType> enum_cast(const char* data, size_t dataSize)
{
    auto value = details::enum_cast<EnumType>(data, dataSize);
    if (value == EnumType::EnumCount)
    {
        throw Exception("Unknown {} enum value: {}", typeid(EnumType).name(), std::string(data, dataSize));
    }

    return value;
}

template <typename EnumType>
constexpr IfEndsWithCount<EnumType> enum_cast(const char* data)
{
    auto value = details::enum_cast<EnumType>(data);
    if (value == EnumType::EnumCount)
    {
        throw Exception("Unknown {} enum value: {}", typeid(EnumType).name(), data);
    }

    return value;
}

template <typename EnumType>
constexpr IfEndsWithUnknown<EnumType> enum_cast(const char* data, size_t dataSize)
{
    return details::enum_cast<EnumType>(data, dataSize);
}

template <typename EnumType>
constexpr IfEndsWithUnknown<EnumType> enum_cast(const char* data)
{
    return details::enum_cast<EnumType>(data);
}

template <typename EnumType>
constexpr const char* string_cast(EnumType value) noexcept
{
    return std::get<0>(lut<EnumType>()[enum_value(value)]);
}

template <typename EnumType>
constexpr bool enumCorrectNess()
{
    for (size_t i = 0; i < lut<EnumType>().size(); ++i)
    {
        // Check that the enums appear in the correct order
        if (std::get<1>(lut<EnumType>().at(i)) != static_cast<EnumType>(i))
        {
            return false;
        }
    }

    return true;
}

#define ADD_ENUM_MAP(enumType, lutVar) \
    template<> constexpr const EnumMap<enumType>& lut<enumType>() { return lutVar; } \
    static_assert(enumCorrectNess<enumType>(), #enumType " enum converion not correctly ordered or missing entries");
}
