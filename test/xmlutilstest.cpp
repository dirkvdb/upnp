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

#include "gtest/gtest.h"
#include <iostream>
#include <algorithm>


using namespace utils;
using namespace testing;

#include "upnp/upnpxmlutils.h"
#include "upnp/upnpitem.h"

namespace upnp
{
namespace test
{

static const std::string testXML =
"<?xml version=\"1.0\"?>"
"<scpd"
"  xmlns=\"urn:schemas-upnp-org:service-1-0\">"
"    <specVersion>"
"        <major>1</major>"
"        <minor>0</minor>"
"    </specVersion>"
"    <actionList>"
"        <action>"
"            <name>GetVolume</name>"
"            <argumentList>"
"                <argument>"
"                    <name>InstanceID</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>Channel</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>CurrentVolume</name>"
"                    <direction>out</direction>"
"                    <relatedStateVariable>Volume</relatedStateVariable>"
"                </argument>"
"            </argumentList>"
"        </action>"
"        <action>"
"            <name>SetVolume</name>"
"            <argumentList>"
"                <argument>"
"                    <name>InstanceID</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_InstanceID</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>Channel</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>A_ARG_TYPE_Channel</relatedStateVariable>"
"                </argument>"
"                <argument>"
"                    <name>DesiredVolume</name>"
"                    <direction>in</direction>"
"                    <relatedStateVariable>Volume</relatedStateVariable>"
"                </argument>"
"            </argumentList>"
"        </action>"
"    </actionList>"
"    <serviceStateTable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>PresetNameList</name>"
"            <dataType>string</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"yes\">"
"            <name>LastChange</name>"
"            <dataType>string</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>Mute</name>"
"            <dataType>boolean</dataType>"
"        </stateVariable>"
"        <stateVariable sendEvents=\"no\">"
"            <name>Volume</name>"
"            <dataType>ui2</dataType>"
"            <allowedValueRange>"
"                <minimum>0</minimum>"
"                <maximum>100</maximum>"
"                <step>1</step>"
"            </allowedValueRange>"
"        </stateVariable>"
"    </serviceStateTable>"
"</scpd>";


class XmlUtilsTest : public Test
{
public:
    virtual ~XmlUtilsTest() {}

protected:
    void SetUp()
    {
        doc = ixmlParseBuffer(testXML.c_str());
    }

    xml::Document createRValueDoc()
    {
        return xml::Document("<doc></doc>");
    }

    xml::Document    doc;
};

TEST_F(XmlUtilsTest, moveOperators)
{
    xml::Document doc1 = createRValueDoc();
    ASSERT_NE(nullptr, static_cast<IXML_Document*>(doc1));
    ASSERT_NE(nullptr, static_cast<IXML_Node*>(doc1));

    xml::Document doc2(createRValueDoc());
    ASSERT_NE(nullptr, static_cast<IXML_Document*>(doc2));
    ASSERT_NE(nullptr, static_cast<IXML_Node*>(doc2));
}

TEST_F(XmlUtilsTest, documentGetChildElementValue)
{
    const std::string xml = "<allowedValueRange>TestValue</allowedValueRange>";

    xml::Document doc(ixmlParseBuffer(xml.c_str()));
    EXPECT_EQ(std::string("TestValue"), doc.getChildNodeValue("allowedValueRange"));
}

TEST_F(XmlUtilsTest, elementGetChildElementValue)
{
    const std::string xml =
    "<allowedValueRange>"
    "    <minimum>0</minimum>"
    "    <maximum>100</maximum>"
    "    <step>1</step>"
    "</allowedValueRange>";

    xml::Document doc(ixmlParseBuffer(xml.c_str()));
    xml::Element node = doc.getFirstChild();
    EXPECT_EQ(std::string("0"),     node.getChildNodeValue("minimum"));
    EXPECT_EQ(std::string("100"),   node.getChildNodeValue("maximum"));
    EXPECT_EQ(std::string("1"),     node.getChildNodeValue("step"));
}

TEST_F(XmlUtilsTest, getStateVariablesFromDescription)
{
    auto vars = xml::utils::getStateVariablesFromDescription(doc);

    auto iter = std::find_if(vars.begin(), vars.end(), [] (const StateVariable& var) {
        return var.name == "LastChange";
    });

    ASSERT_NE(vars.end(), iter);
    EXPECT_STREQ("LastChange", iter->name.c_str());
    EXPECT_STREQ("string", iter->dataType.c_str());
    EXPECT_EQ(nullptr, iter->valueRange);

    iter = std::find_if(vars.begin(), vars.end(), [] (const StateVariable& var) {
        return var.name == "Volume";
    });

    ASSERT_NE(vars.end(), iter);
    EXPECT_STREQ("Volume", iter->name.c_str());
    EXPECT_STREQ("ui2", iter->dataType.c_str());
    EXPECT_NE(nullptr, iter->valueRange);
    EXPECT_EQ(0, iter->valueRange->minimumValue);
    EXPECT_EQ(100, iter->valueRange->maximumValue);
    EXPECT_EQ(1U, iter->valueRange->step);
}

TEST_F(XmlUtilsTest, itemToDocument)
{
    Item item("0");
    item.addMetaData(Property::Album, "An album");
    item.addMetaData(Property::Class, "object.container.album.musicAlbum");

    auto doc = xml::utils::getItemDocument(item);
    auto didl = doc.getElementsByTagName("DIDL-Lite").getNode(0);
    xml::Element itemElem = didl.getFirstChild();

    EXPECT_EQ(0U, itemElem.getAttributeAsNumeric<uint32_t>("id"));
    EXPECT_STREQ("", itemElem.getAttribute("parentID").c_str());
    EXPECT_STREQ("An album", itemElem.getChildNodeValue(toString(Property::Album)).c_str());
    EXPECT_STREQ("object.container.album.musicAlbum", itemElem.getChildNodeValue(toString(Property::Class)).c_str());
}

}
}
