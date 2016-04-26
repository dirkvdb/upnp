#include <benchmark/benchmark.h>

#include "upnp/upnp.xml.parseutils.h"

#include "testxmls.h"
#include "rapidxml_print.hpp"

namespace upnp
{
namespace benchmark
{

using namespace ::benchmark;

class XmlParseUtils : public benchmark::Fixture
{

};

BENCHMARK_F(XmlParseUtils, ParseContainer)(State& st)
{
    while (st.KeepRunning())
    {
        ContentDirectory::ActionResult result;
        auto xml = xml::parseBrowseResult(test::testxmls::innerBrowseResponseItems, result);
        DoNotOptimize(xml::parseContainers(xml));
        DoNotOptimize(xml::parseItems(xml));
    }
}

}
}
