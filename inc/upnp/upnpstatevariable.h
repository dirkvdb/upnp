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

#ifndef UPNP_STATE_VARIABLE_H
#define UPNP_STATE_VARIABLE_H

#include <string>
#include <cinttypes>
#include <memory>
#include <iostream>

namespace upnp
{

class StateVariable
{
public:
    StateVariable() {}

    StateVariable(const StateVariable& var)
    : name(var.name)
    , dataType(var.dataType)
    {
        if (var.valueRange)
        {
            valueRange.reset(new ValueRange(*var.valueRange));
        }
    }
    
    StateVariable(StateVariable&& var)
    : name(std::move(var.name))
    , dataType(std::move(var.dataType))
    , valueRange(std::move(var.valueRange))
    {
    }

    class ValueRange
    {
    public:
        int32_t      minimumValue = 0;
        int32_t      maximumValue = 0;
        uint32_t     step = 0;
    };

    std::string                 name;
    std::string                 dataType;
    bool                        sendsEvents = false;
    std::unique_ptr<ValueRange> valueRange;
};

inline std::ostream& operator<< (std::ostream& os, const StateVariable::ValueRange& range)
{
    return os << "ValueRange: [" << range.minimumValue << "-" << range.maximumValue << "] step = " << range.step;
}

}


#endif
