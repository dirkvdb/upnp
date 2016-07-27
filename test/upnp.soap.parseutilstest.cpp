#include <gtest/gtest.h>
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

TEST(SoapParseUtils, SoapFault)
{
    auto faultXml =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "  <s:Body>"
        "    <s:Fault>"
        "      <faultcode>s:Client</faultcode>"
        "      <faultstring>UPnPError</faultstring>"
        "      <detail>"
        "        <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
        "          <errorCode>718</errorCode>"
        "          <errorDescription>ConflictInMappingEntry</errorDescription>"
        "        </UPnPError>"
        "      </detail>"
        "    </s:Fault>"
        "  </s:Body>"
        "</s:Envelope>"s;

    auto fault = soap::parseFault(faultXml);
    EXPECT_EQ(718u, fault.errorCode);
    EXPECT_EQ("ConflictInMappingEntry", fault.errorDescription);
}

TEST(SoapParseUtils, SoapFaultNoDescription)
{
    auto faultXml =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "  <s:Body>"
        "    <s:Fault>"
        "      <faultcode>s:Client</faultcode>"
        "      <faultstring>UPnPError</faultstring>"
        "      <detail>"
        "        <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">"
        "          <errorCode>718</errorCode>"
        "        </UPnPError>"
        "      </detail>"
        "    </s:Fault>"
        "  </s:Body>"
        "</s:Envelope>"s;

    auto fault = soap::parseFault(faultXml);
    EXPECT_EQ(718u, fault.errorCode);
    EXPECT_TRUE(fault.errorDescription.empty());
}

}
}
