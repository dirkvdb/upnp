#include <gtest/gtest.h>
#include <pugixml.hpp>
#include <sstream>

#include "upnp.soap.parseutils.h"

namespace upnp
{
namespace test
{

using namespace std::string_literals;
using namespace std::chrono_literals;

TEST(SoapParseUtils, SoapTimeout)
{
    EXPECT_EQ(0, soap::parseTimeout("Second-infinite").count());
    EXPECT_EQ(1800, soap::parseTimeout("Second-1800").count());
}

}
}
