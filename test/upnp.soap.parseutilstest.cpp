#include <catch.hpp>
#include <pugixml.hpp>
#include <sstream>

#include "upnp.soap.parseutils.h"

namespace upnp
{
namespace test
{

using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("Parse soap timeout", "[PARSE]")
{
    CHECK(soap::parseTimeout("Second-infinite").count() == 0);
    CHECK(soap::parseTimeout("Second-1800").count() == 1800);
}

}
}
