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

#include "gsl/span.h"
#include "upnp/upnptypes.h"

namespace upnp
{

template <typename EnumType>
constexpr typename std::underlying_type<EnumType>::type enum_value(EnumType e)
{
    return static_cast<typename std::underlying_type<EnumType>::type>(e);
}

template<typename T>
constexpr const std::tuple<const char*, T>* lut()
{
    static_assert(!std::is_enum<T>::value, "No lookup table provided for type");
    return nullptr;
}

namespace details
{

template <typename EnumType, int count>
constexpr bool enumCorrectNess()
{
    for (uint32_t i = 0; i < count; ++i)
    {
        // Check that the enums appear in the correct order
        if (std::get<1>(lut<EnumType>()[i]) != static_cast<EnumType>(i))
        {
            return false;
        }
    }

    return true;
}

template <typename EnumType, int count>
constexpr const char* toString(EnumType value) noexcept
{
    assert(enum_value(value) < count);
    return std::get<0>(lut<EnumType>()[enum_value(value)]);
}

}

template <typename EnumType, EnumType count = EnumType::EnumCount>
constexpr EnumType fromString(const char* data, size_t dataSize)
{
    for (uint32_t i = 0; i < enum_value(count); ++i)
    {
        if (strncmp(std::get<0>(lut<EnumType>()[i]), data, dataSize) == 0)
        {
            return static_cast<EnumType>(i);
        }
    }

    throw Exception("Unknown {} enum value: {}", typeid(EnumType).name(), std::string(data, dataSize));
}

template <typename EnumType, EnumType count = EnumType::EnumCount>
constexpr EnumType fromString(const char* data)
{
    for (uint32_t i = 0; i < enum_value(count); ++i)
    {
        if (strcmp(std::get<0>(lut<EnumType>()[i]), data) == 0)
        {
            return static_cast<EnumType>(i);
        }
    }

    throw Exception("Unknown ContentDirectory enum: {}", data);
}

template <typename EnumType, int count = enum_value(EnumType::Unknown)>
constexpr EnumType fromString(const char* data, size_t dataSize)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        if (strncmp(std::get<0>(lut<EnumType>()[i]), data, dataSize) == 0)
        {
            return static_cast<EnumType>(i);
        }
    }

    return EnumType::Unknown;
}

template <typename EnumType>
constexpr EnumType fromString(const std::string& value)
{
    return fromString<EnumType>(value.c_str(), value.size());
}

template <typename EnumType, EnumType count = EnumType::EnumCount>
constexpr const char* toString(EnumType value) noexcept
{
    return details::toString<EnumType, enum_value(count)>(value);
}

template <typename EnumType, int count = enum_value(EnumType::Unknown)>
constexpr const char* toString(EnumType value) noexcept
{
    return details::toString<EnumType, count>(value);
}

template <typename EnumType, EnumType count = EnumType::EnumCount>
constexpr bool enumCorrectNess()
{
    return details::enumCorrectNess<EnumType, enum_value(count)>();
}

template <typename EnumType, int count = enum_value(EnumType::Unknown)>
constexpr bool enumCorrectNess()
{
    return details::enumCorrectNess<EnumType, count>();
}

}
