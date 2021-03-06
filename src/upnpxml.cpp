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

#include "upnp/upnpxml.h"

#include "utils/log.h"
#include "utils/numericoperations.h"
#include "upnp/upnptypes.h"

#include <cstring>
#include <stdexcept>

using namespace utils;

namespace upnp
{
namespace xml
{

Node::Node()
: m_pNode(nullptr)
{
}

Node::Node(IXML_Node* pNode)
: m_pNode(pNode)
{
}

Node::Node(const Node& node)
: m_pNode(node.m_pNode)
{
}

Node::~Node()
{
}

Node::operator IXML_Node*() const
{
    return m_pNode;
}

Node::operator bool() const
{
    return m_pNode != nullptr;
}

bool Node::operator== (const Node& other) const
{
    return m_pNode == other.m_pNode;
}

bool Node::operator!= (const Node& other) const
{
    return m_pNode != other.m_pNode;
}

std::string Node::getName() const
{
    const char* pStr = ixmlNode_getNodeName(m_pNode);
    if (!pStr)
    {
        throw Exception("Failed to get node name");
    }

    return pStr;
}

std::string Node::getValue() const
{
    const char* pStr = ixmlNode_getNodeValue(m_pNode);
    if (!pStr)
    {
        return "";
    }

    return pStr;
}

Node Node::getParent() const
{
    return ixmlNode_getParentNode(m_pNode);
}

Node Node::getFirstChild() const
{
    Node node = ixmlNode_getFirstChild(m_pNode);
    if (!node)
    {
        throw Exception("Failed to get first child node from node: {}", getName());
    }

    return node;
}

NodeList Node::getChildNodes() const
{
    NodeList children = ixmlNode_getChildNodes(m_pNode);
    if (!children)
    {
        throw Exception("Failed to get childNodes from node: {}", getName());
    }

    return children;
}

Node Node::getChildNode(const std::string& tagName) const
{
    for (auto& node : getChildNodes())
    {
        if (node.getName() == tagName)
        {
            return node;
        }
    }

    throw Exception("No child node found with name {}", tagName);
}

std::string Node::getChildNodeValue(const std::string& tagName) const
{
    auto node = getChildNode(tagName);
    Node textNode = ixmlNode_getFirstChild(node);
    if (!textNode)
    {
        // No text node, so empty
        return "";
    }

    return textNode.getValue();
}

Document Node::getOwnerDocument() const
{
    return Document(ixmlNode_getOwnerDocument(m_pNode), Document::NoOwnership);
}

void Node::appendChild(Node& node)
{
    if (IXML_SUCCESS != ixmlNode_appendChild(m_pNode, node))
    {
        throw Exception("Failed to append child node: {}", node.getName());
    }
}

std::string Node::toString() const
{
    String str(ixmlPrintNode(m_pNode));
    if (!str)
    {
        throw Exception("Failed to convert node to string");
    }

    return str;
}

void Node::setNodePointer(IXML_Node* pNode)
{
    m_pNode = pNode;
}

Document::Document()
: m_pDoc(ixmlDocument_createDocument())
, m_Ownership(TakeOwnership)
{
    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
}

Document::Document(const std::string& xml)
: m_pDoc(ixmlParseBuffer(xml.c_str()))
, m_Ownership(TakeOwnership)
{
    if (!m_pDoc)
    {
        throw Exception("Invalid xml document string received");
    }

    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
}

Document::Document(IXML_Document* pDoc, OwnershipType ownership)
: Node(reinterpret_cast<IXML_Node*>(pDoc))
, m_pDoc(pDoc)
, m_Ownership(ownership)
{
}

Document::Document(const Document& doc)
: Node()
, m_pDoc(nullptr)
, m_Ownership(TakeOwnership)
{
    // this copy constructor is only meant to be used in unit tests where the use of gmock requries copy constructors
    // real code sjould avoid copies of the document for performance reasons

    if (doc)
    {
        //m_pDoc = doc.m_pDoc;
        //setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
        //m_Ownership = NoOwnership;
        //return;

        m_pDoc = ixmlDocument_createDocument();
        setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));

        //log::warn("Document copy constructor is implmented for gmock compatibility, should not get executed in code for performance reasons %", m_pDoc);

        try
        {
            for (auto& node : doc.getChildNodes())
            {
                IXML_Node* pNode = nullptr;
                if (IXML_SUCCESS != ixmlDocument_importNode(m_pDoc, node, TRUE, &pNode))
                {
                    throw Exception("Failed to clone xml document");
                }

                ixmlNode_appendChild(reinterpret_cast<IXML_Node*>(static_cast<IXML_Document*>(m_pDoc)), pNode);
            }
        }
        catch (std::exception&)
        {
            ixmlDocument_free(m_pDoc);
            m_pDoc = nullptr;
            throw;
        }
    }
}

Document::Document(Document&& doc)
: Node(reinterpret_cast<IXML_Node*>(doc.m_pDoc))
, m_pDoc(doc.m_pDoc)
, m_Ownership(doc.m_Ownership)
{
	doc.m_pDoc = nullptr;
    doc.m_Ownership = NoOwnership;
}

Document::~Document()
{
    if (TakeOwnership == m_Ownership)
    {
        ixmlDocument_free(m_pDoc);
    }
}

Document& Document::operator= (Document&& other)
{
    m_pDoc 				= other.m_pDoc;
    m_Ownership 		= other.m_Ownership;
    other.m_pDoc		= nullptr;
    other.m_Ownership 	= NoOwnership;

    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));

    return *this;
}

Document& Document::operator= (IXML_Document* pDoc)
{
    m_pDoc = pDoc;
    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
    return *this;
}

Document::operator IXML_Document*() const
{
    return m_pDoc;
}

Document::operator bool() const
{
    return m_pDoc != nullptr;
}

NodeList Document::getElementsByTagName(const std::string& tagName) const
{
    return NodeList(ixmlDocument_getElementsByTagName(m_pDoc, tagName.c_str()));
}

std::string Document::getChildNodeValueRecursive(const std::string& tagName) const
{
    NodeList nodeList = getElementsByTagName(tagName);
    if (!nodeList || nodeList.size() == 0)
    {
        throw Exception("Failed to get document subelement value with tag: {}", tagName);
    }

    auto node = nodeList.getNode(0);
    try
    {
        return node.getFirstChild().getValue();
    }
    catch (std::exception)
    {
        // empty node value
        return "";
    }
}

Node Document::createNode(const std::string& value)
{
    Node node(ixmlDocument_createTextNode(m_pDoc, value.c_str()));
    if (!node)
    {
        throw Exception("Failed to create document node: {}", value);
    }

    return node;
}

Element Document::createElement(const std::string& name)
{
    Element elem(ixmlDocument_createElement(m_pDoc, name.c_str()));
    if (!elem)
    {
        throw Exception("Failed to create document element: {}", name);
    }

    return elem;
}

Element Document::createElementNamespaced(const std::string& nameSpace, const std::string& name)
{
    Element elem(ixmlDocument_createElementNS(m_pDoc, nameSpace.c_str(), name.c_str()));
    if (!elem)
    {
        throw Exception("Failed to create namespaced document element: {}", name);
    }

    return elem;
}

NodeList::NodeList()
: m_pList(nullptr)
{
}

NodeList::NodeList(IXML_NodeList* pList)
: m_pList(pList)
{
}

NodeList::~NodeList()
{
    ixmlNodeList_free(m_pList);
}

NodeList& NodeList::operator= (IXML_NodeList* pList)
{
    m_pList = pList;
    return *this;
}

NodeList::operator IXML_NodeList*() const
{
    return m_pList;
}

Node NodeList::operator[] (uint64_t index) const
{
    return getNode(index);
}

NodeList::operator bool() const
{
    return m_pList != nullptr;
}

Node NodeList::getNode(uint64_t index) const
{
    Node node = ixmlNodeList_item(m_pList, index);
    if (!node)
    {
        throw Exception("Failed to find node in nodelist on index: {}", index);
    }

    return node;
}

uint64_t NodeList::size() const
{
    return ixmlNodeList_length(m_pList);
}

NamedNodeMap Node::getAttributes() const
{
    NamedNodeMap nodeMap = ixmlNode_getAttributes(m_pNode);
    if (!nodeMap)
    {
        throw Exception("Failed to get attribute map from node: {}", getName());
    }

    return nodeMap;
}

NamedNodeMap::NamedNodeMap()
: m_pNodeMap(nullptr)
{
}

NamedNodeMap::NamedNodeMap(IXML_NamedNodeMap* pNodeMap)
: m_pNodeMap(pNodeMap)
{
}

NamedNodeMap::~NamedNodeMap()
{
    ixmlNamedNodeMap_free(m_pNodeMap);
}

uint64_t NamedNodeMap::size() const
{
    return ixmlNamedNodeMap_getLength(m_pNodeMap);
}

Node NamedNodeMap::getNode(uint64_t index) const
{
    Node node = ixmlNamedNodeMap_item(m_pNodeMap, index);
    if (!node)
    {
        throw Exception("Failed to get node from named node map");
    }

    return node;
}

Node NamedNodeMap::getNode(const std::string &name) const
{
    Node node = ixmlNamedNodeMap_getNamedItem(m_pNodeMap, name.c_str());
    if (!node)
    {
        throw Exception("Failed to get node from named node map: {}", name);
    }

    return node;
}

NamedNodeMap::operator IXML_NamedNodeMap*() const
{
    return m_pNodeMap;
}

NamedNodeMap::operator bool() const
{
    return m_pNodeMap != nullptr;
}

Node NamedNodeMap::operator[] (uint64_t index) const
{
    return getNode(index);
}

Element::Element()
: m_pElement(nullptr)
{
}

Element::Element(IXML_Element* pElement)
: Node(reinterpret_cast<IXML_Node*>(pElement))
, m_pElement(pElement)
{
}

Element::Element(const Node& node)
: Node(node)
, m_pElement(reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(*this)))
{
}

// this move constructor allows a move of an XmlNode to an XmlElement, it upcasts the node
Element::Element(Node&& node)
: Node(std::forward<Node>(node))
, m_pElement(reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(*this)))
{
}

// this assignment operator allows a copy of an XmlNode to an XmlElement, it upcasts the node
Element& Element::operator= (Node& node)
{
    setNodePointer(static_cast<IXML_Node*>(node));
    m_pElement = reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(node));
    return *this;
}

Element::operator IXML_Element*() const
{
    return m_pElement;
}

Element::operator bool() const
{
    return m_pElement != nullptr;
}

std::string Element::getName() const
{
    return ixmlElement_getTagName(m_pElement);
}

std::string Element::getValue() const
{
    try
    {
        return getFirstChild().getValue();
    }
    catch (std::exception&)
    {
        return "";
    }
}

std::string Element::getAttribute(const std::string& attr)
{
    const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
    if (!pAttr)
    {
        throw Exception("Failed to get attribute from element: {}", attr);
    }

    return pAttr;
}

std::string Element::getAttributeOptional(const std::string& attr, const std::string& defaultValue)
{
    const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
    return pAttr ? pAttr : defaultValue;
}

void Element::addAttribute(const std::string& name, const std::string& value)
{
    auto doc = getOwnerDocument();
    auto attr = ixmlDocument_createAttribute(doc, name.c_str());
    ixmlNode_setNodeValue(reinterpret_cast<IXML_Node*>(attr), value.c_str());

    IXML_Attr* pAttr;
    ixmlElement_setAttributeNode(m_pElement, attr, &pAttr);
}

NodeList Element::getElementsByTagName(const std::string& tagName)
{
    NodeList list = ixmlElement_getElementsByTagName(m_pElement, tagName.c_str());
    if (!list)
    {
        throw Exception("Failed to get element subelements with tag: {}", tagName);
    }

    return list;
}

Element Element::getChildElement(const std::string& tagName)
{
    return getChildNode(tagName);
}

String::String(DOMString str)
: m_String(str)
{
}

String::~String()
{
    ixmlFreeDOMString(m_String);
}

String::operator std::string() const
{
    return std::string(m_String);
}

String::operator DOMString() const
{
    return m_String;
}

String::operator bool() const
{
    return m_String != nullptr;
}

}
}
