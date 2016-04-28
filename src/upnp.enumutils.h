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

#include <tuple>
#include <type_traits>

#include "upnp/upnptypes.h"

namespace upnp
{

template <typename EnumType>
constexpr typename std::underlying_type_t<EnumType> enum_value(EnumType e)
{
    return static_cast<typename std::underlying_type_t<EnumType>>(e);
}

template<typename T>
constexpr const std::tuple<const char*, T>* lut()
{
    static_assert(!std::is_enum<T>::value, "No lookup table provided for type");
    return nullptr;
}

template<typename EnumType>
using IfEndsWithCount = typename std::enable_if_t<std::is_integral<decltype(enum_value(EnumType::EnumCount))>::value>;

template<typename EnumType>
using IfEndsWithUnknown = typename std::enable_if_t<std::is_integral<decltype(enum_value(EnumType::Unknown))>::value>;

namespace details
{

template <typename EnumType, std::underlying_type_t<EnumType> Count>
constexpr bool enumCorrectNess()
{
    for (decltype(Count) i = 0; i < Count; ++i)
    {
        // Check that the enums appear in the correct order
        if (std::get<1>(lut<EnumType>()[i]) != static_cast<EnumType>(i))
        {
            return false;
        }
    }

    return true;
}

template <typename EnumType, std::underlying_type_t<EnumType> Count>
constexpr std::underlying_type_t<EnumType> fromString(const char* data)
{
    for (decltype(Count) i = 0; i < Count; ++i)
    {
        if (strcmp(std::get<0>(lut<EnumType>()[i]), data) == 0)
        {
            return static_cast<EnumType>(i);
        }
    }

    return Count;
}

template <typename EnumType, std::underlying_type_t<EnumType> Count>
constexpr std::underlying_type_t<EnumType> fromString(const char* data, size_t dataSize)
{
    for (decltype(Count) i = 0; i < Count; ++i)
    {
        if (strncmp(std::get<0>(lut<EnumType>()[i]), data, dataSize) == 0)
        {
            return i;
        }
    }

    return Count;
}

template <typename EnumType, int Count>
constexpr const char* toString(EnumType value) noexcept
{
    assert(enum_value(value) < Count);
    return std::get<0>(lut<EnumType>()[enum_value(value)]);
}

template <typename EnumType>
constexpr EnumType handleFromStringUnknown(std::underlying_type_t<EnumType> index) noexcept
{
    if (index == enum_value(EnumType::Unknown))
    {
        return EnumType::Unknown;
    }

    return static_cast<EnumType>(index);
}

}

template <typename EnumType, IfEndsWithCount<EnumType>* = nullptr>
constexpr EnumType fromString(const char* data, size_t dataSize)
{
    auto index = details::fromString<EnumType, enum_value(EnumType::EnumCount)>(data, dataSize);
    if (index == enum_value(EnumType::EnumCount))
    {
        throw Exception("Unknown {} enum value: {}", typeid(EnumType).name(), std::string(data, dataSize));
    }

    return static_cast<EnumType>(index);
}

template <typename EnumType, IfEndsWithCount<EnumType>* = nullptr>
constexpr EnumType fromString(const char* data)
{
    auto index = details::fromString<EnumType, enum_value(EnumType::EnumCount)>(data);
    if (index == enum_value(EnumType::EnumCount))
    {
        throw Exception("Unknown {} enum value: {}", typeid(EnumType).name(), data);
    }

    return static_cast<EnumType>(index);
}

template <typename EnumType, IfEndsWithUnknown<EnumType>* = nullptr>
constexpr EnumType fromString(const char* data, size_t dataSize)
{
    return details::handleFromStringUnknown<EnumType>(details::fromString<EnumType, enum_value(EnumType::Unknown)>(data, dataSize));
}

template <typename EnumType, IfEndsWithUnknown<EnumType>* = nullptr>
constexpr EnumType fromString(const char* data)
{
    return details::handleFromStringUnknown<EnumType>(details::fromString<EnumType, enum_value(EnumType::Unknown)>(data));
}

template <typename EnumType, IfEndsWithCount<EnumType>* = nullptr>
constexpr const char* toString(EnumType value) noexcept
{
    return details::toString<EnumType, enum_value(EnumType::EnumCount)>(value);
}

template <typename EnumType, IfEndsWithUnknown<EnumType>* = nullptr>
constexpr const char* toString(EnumType value) noexcept
{
    return details::toString<EnumType, enum_value(EnumType::Unknown)>(value);
}

template <typename EnumType, IfEndsWithCount<EnumType>* = nullptr>
constexpr bool enumCorrectNess()
{
    return details::enumCorrectNess<EnumType, enum_value(EnumType::EnumCount)>();
}

template <typename EnumType, IfEndsWithUnknown<EnumType>* = nullptr>
constexpr bool enumCorrectNess()
{
    return details::enumCorrectNess<EnumType, enum_value(EnumType::Unknown)>();
}

#define ADD_ENUM_MAP(enumType, lutVar) \
    template<> constexpr const std::tuple<const char*, enumType>* lut<enumType>() { return lutVar; } \
    static_assert(enumCorrectNess<enumType>(), #enumType " enum converion not correctly ordered or missing entries");

}
