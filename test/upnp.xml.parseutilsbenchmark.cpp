#include <benchmark/benchmark.h>
#include <unordered_map>

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
        auto xml = xml::parseBrowseResult(test::testxmls::browseResponseItems, result);
        DoNotOptimize(xml::parseContainers(xml));
        DoNotOptimize(xml::parseItems(xml));
    }
}

BENCHMARK_F(XmlParseUtils, EnumConversionMap)(State& st)
{
    static std::map<std::string, ContentDirectory::Variable> lookup = {
        { "ContainerUpdateIDs", ContentDirectory::Variable::ContainerUpdateIDs },
        { "TransferIDs", ContentDirectory::Variable::TransferIDs },
        { "SystemUpdateID", ContentDirectory::Variable::SystemUpdateID },
        { "A_ARG_TYPE_ObjectID", ContentDirectory::Variable::ArgumentTypeObjectID },
        { "A_ARG_TYPE_Result", ContentDirectory::Variable::ArgumentTypeResult },
        { "A_ARG_TYPE_SearchCriteria", ContentDirectory::Variable::ArgumentTypeSearchCriteria },
        { "A_ARG_TYPE_Flag", ContentDirectory::Variable::ArgumentTypeBrowseFlag },
        { "A_ARG_TYPE_Filter", ContentDirectory::Variable::ArgumentTypeFilter },
        { "A_ARG_TYPE_SortCriteria", ContentDirectory::Variable::ArgumentTypeSortCriteria },
        { "A_ARG_TYPE_Index", ContentDirectory::Variable::ArgumentTypeIndex },
        { "A_ARG_TYPE_Count", ContentDirectory::Variable::ArgumentTypeCount },
        { "A_ARG_TYPE_UpdateID", ContentDirectory::Variable::ArgumentTypeUpdateID },
        { "SearchCapabilities", ContentDirectory::Variable::SearchCapabilities },
        { "SortCapabilities", ContentDirectory::Variable::SortCapabilities }
    };

    while (st.KeepRunning())
    {
        DoNotOptimize(lookup["A_ARG_TYPE_ObjectID"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Result"]);
        DoNotOptimize(lookup["A_ARG_TYPE_SearchCriteria"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Flag"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Filter"]);
        DoNotOptimize(lookup["A_ARG_TYPE_SortCriteria"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Index"]);
    }
}

BENCHMARK_F(XmlParseUtils, EnumConversionUnorderedMap)(State& st)
{
    static std::unordered_map<std::string, ContentDirectory::Variable> lookup = {
        { "ContainerUpdateIDs", ContentDirectory::Variable::ContainerUpdateIDs },
        { "TransferIDs", ContentDirectory::Variable::TransferIDs },
        { "SystemUpdateID", ContentDirectory::Variable::SystemUpdateID },
        { "A_ARG_TYPE_ObjectID", ContentDirectory::Variable::ArgumentTypeObjectID },
        { "A_ARG_TYPE_Result", ContentDirectory::Variable::ArgumentTypeResult },
        { "A_ARG_TYPE_SearchCriteria", ContentDirectory::Variable::ArgumentTypeSearchCriteria },
        { "A_ARG_TYPE_Flag", ContentDirectory::Variable::ArgumentTypeBrowseFlag },
        { "A_ARG_TYPE_Filter", ContentDirectory::Variable::ArgumentTypeFilter },
        { "A_ARG_TYPE_SortCriteria", ContentDirectory::Variable::ArgumentTypeSortCriteria },
        { "A_ARG_TYPE_Index", ContentDirectory::Variable::ArgumentTypeIndex },
        { "A_ARG_TYPE_Count", ContentDirectory::Variable::ArgumentTypeCount },
        { "A_ARG_TYPE_UpdateID", ContentDirectory::Variable::ArgumentTypeUpdateID },
        { "SearchCapabilities", ContentDirectory::Variable::SearchCapabilities },
        { "SortCapabilities", ContentDirectory::Variable::SortCapabilities }
    };

    while (st.KeepRunning())
    {
        DoNotOptimize(lookup["A_ARG_TYPE_ObjectID"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Result"]);
        DoNotOptimize(lookup["A_ARG_TYPE_SearchCriteria"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Flag"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Filter"]);
        DoNotOptimize(lookup["A_ARG_TYPE_SortCriteria"]);
        DoNotOptimize(lookup["A_ARG_TYPE_Index"]);
    }
}

BENCHMARK_F(XmlParseUtils, EnumConversionStringCompare)(State& st)
{
    using namespace ContentDirectory;
    while (st.KeepRunning())
    {
        DoNotOptimize(variableFromString("A_ARG_TYPE_ObjectID"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Result"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_SearchCriteria"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Flag"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Filter"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_SortCriteria"));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Index"));
    }
}

BENCHMARK_F(XmlParseUtils, EnumConversionStringCompareRawString)(State& st)
{
    using namespace ContentDirectory;
    while (st.KeepRunning())
    {
        DoNotOptimize(variableFromString("A_ARG_TYPE_ObjectID", 19));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Result", 17));
        DoNotOptimize(variableFromString("A_ARG_TYPE_SearchCriteria", 25));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Flag", 15));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Filter", 17));
        DoNotOptimize(variableFromString("A_ARG_TYPE_SortCriteria", 23));
        DoNotOptimize(variableFromString("A_ARG_TYPE_Index", 16));
    }
}

}
}
