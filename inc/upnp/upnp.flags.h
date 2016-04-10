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

#include <type_traits>

namespace upnp
{

template<typename T, typename ValueType = typename std::underlying_type<T>::type>
class Flags
{
public:
    Flags() = default;
    Flags(const Flags<T, ValueType>& other) = default;
    Flags(Flags<T, ValueType>&& other) = default;

    Flags(T flag)
    : m_flags(getValue(flag))
    {
    }

    Flags(ValueType flags)
    : m_flags(flags)
    {
    }

    template <typename... Flag>
    Flags(Flag&&... flags)
    : m_flags(getValue(std::forward<Flag&&>(flags)...))
    {
    }

    operator ValueType() const noexcept
    {
        return m_flags;
    }
    
    bool isSet(T flag) const noexcept
    {
        return (m_flags & getValue(flag)) != 0;
    }

private:
    ValueType getValue(T flag) const noexcept
    {
        return static_cast<ValueType>(flag);
    }

    template <typename... Flag>
    ValueType getValue(T flag, Flag&&... flags) const noexcept
    {
        return getValue(flag) | getValue(std::forward<T>(flags)...);
    }

    ValueType m_flags = 0;
};

template <typename T>
inline Flags<T> operator | (T lhs, T rhs)
{
    return Flags<T>(lhs, rhs);
}


}
