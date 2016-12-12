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

#include "utils/signal.h"

#include <gtest/gtest.h>
#include <iostream>
#include <algorithm>

#include "upnp/upnp.utils.h"

using namespace utils;
using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace upnp
{
namespace test
{

TEST(UtilsTest, DurationFromString)
{
    EXPECT_EQ(145s, durationFromString(":02:25"));
    EXPECT_EQ(145s, durationFromString("0:02:25"));
    EXPECT_EQ(145s, durationFromString("00:02:25"));
    EXPECT_EQ(145s, durationFromString("00:02:25.00"));
    EXPECT_EQ(3745s, durationFromString("01:02:25.00"));
}

TEST(UtilsTest, DurationToString)
{
    EXPECT_EQ("00:02:25"s, durationToString(145s));
    EXPECT_EQ("01:02:25"s, durationToString(3745s));
}

}
}
