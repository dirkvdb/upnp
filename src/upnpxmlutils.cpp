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

#include "upnp/upnpxmlutils.h"

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/numericoperations.h"

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

Node::Node(Node&& node)
: m_pNode(std::move(node.m_pNode))
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

bool Node::operator == (const Node& other) const
{
    return m_pNode == other.m_pNode;
}

bool Node::operator != (const Node& other) const
{
    return m_pNode != other.m_pNode;
}

std::string Node::getName() const
{
    return ixmlNode_getNodeName(m_pNode);
}

std::string Node::getValue() const
{
    const char* pStr = ixmlNode_getNodeValue(m_pNode);
    if (!pStr)
    {
        throw std::logic_error(std::string("Failed to get node value: ") + getName());
    }
    
    return pStr;
}

std::string Node::toString()
{
    String str(ixmlPrintNode(m_pNode));
    if (!str)
    {
        throw std::logic_error("Failed to convert node to string");
    }
    
    return str;
}

void Node::setNodePointer(IXML_Node* pNode)
{
    m_pNode = pNode;
}

Document::Document()
: m_pDoc(nullptr)
, m_Ownership(NoOwnership)
{
}

Document::Document(const std::string& xml)
: m_pDoc(ixmlParseBuffer(xml.c_str()))
, m_Ownership(TakeOwnership)
{
    if (!m_pDoc)
    {
        throw std::logic_error("Invalid xml document string received");
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
: m_pDoc(nullptr)
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
    
        //log::warn("Document copy constructor is implmented for gmock compatibility, should not get executed in code for performance reaseons");
        
        m_pDoc = ixmlDocument_createDocument();
        setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
        
        try
        {
            NodeList nodeList = doc.getChildNodes();
            uint64_t size = nodeList.size();
            for (uint64_t i = 0; i < size; ++i)
            {
                IXML_Node* pNode = nullptr;
                Node node = nodeList.getNode(i);
                if (IXML_SUCCESS != ixmlDocument_importNode(m_pDoc, node, TRUE, &pNode))
                {
                    throw std::logic_error("Failed to clone xml document");
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
: Node(std::forward<Node>(doc))
, m_pDoc(std::move(doc.m_pDoc))
, m_Ownership(TakeOwnership)
{
}

Document::~Document()
{
    if (TakeOwnership == m_Ownership)
    {
        //log::debug(toString());
        //ixmlDocument_free(m_pDoc);
    }
}

Document& Document::operator= (IXML_Document* pDoc)
{
    m_pDoc = pDoc;
    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
    return *this;
}

IXML_Document** Document::operator &()
{
    return &m_pDoc;
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
    NodeList nodeList = ixmlDocument_getElementsByTagName(m_pDoc, tagName.c_str());
    if (!nodeList)
    {
        throw std::logic_error(std::string("Failed to find tags in document with name: ") + tagName);
    }
    
    return nodeList;
}

std::string Document::getChildElementValue(const std::string& tagName) const
{
    NodeList list = getChildNodes();
    for (uint64_t i = 0; i < list.size(); ++i)
    {
        Node node = list.getNode(i);
        if (node.getName() == tagName)
        {
            return node.getFirstChild().getValue();
        }
    }
    
    throw std::logic_error(std::string("No child element found in document with name ") + tagName);
}

std::string Document::getChildElementValueRecursive(const std::string& tagName) const
{
    NodeList nodeList = getElementsByTagName(tagName);
    if (nodeList.size() == 0)
    {
        throw std::logic_error(std::string("Failed to get document subelement value with tag: ") + tagName);
    }
    
    return nodeList.getNode(0).getFirstChild().getValue();
}

std::string Document::toString() const
{
    String str(ixmlDocumenttoString(m_pDoc));
    if (!str)
    {
        throw std::logic_error("Failed to convert document to string");
    }
    
    return str;
}

NodeList::NodeList()
: m_pList(nullptr)
{
}

NodeList::NodeList(IXML_NodeList* pList)
: m_pList(pList)
{
}

NodeList::NodeList(NodeList&& doc)
: m_pList(std::move(doc.m_pList))
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
        throw std::logic_error(std::string("Failed to find node in nodelist on index: ") + numericops::toString(index));
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
        throw std::logic_error(std::string("Failed to get attribute map from node: ") + getName());
    }
    
    return nodeMap;
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
        throw std::logic_error(std::string("Failed to get first child node from node: ") + getName());
    }
    
    return node;
}

NodeList Node::getChildNodes() const
{
    NodeList children = ixmlNode_getChildNodes(m_pNode);
    if (!children)
    {
        throw std::logic_error(std::string("Failed to get childNodes from node: ") + getName());
    }
    
    return children;
}

NamedNodeMap::NamedNodeMap()
: m_pNodeMap(nullptr)
{
}

NamedNodeMap::NamedNodeMap(IXML_NamedNodeMap* pNodeMap)
: m_pNodeMap(pNodeMap)
{
}

NamedNodeMap::NamedNodeMap(NamedNodeMap&& nodeMap)
: m_pNodeMap(std::move(nodeMap.m_pNodeMap))
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
        throw std::logic_error("Failed to get node from named node map");
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

Element::Element(Element&& node)
: m_pElement(std::move(node.m_pElement))
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

std::string Element::getName()
{
    return ixmlElement_getTagName(m_pElement);
}

std::string Element::getValue()
{
    return getFirstChild().getValue();
}

std::string Element::getAttribute(const std::string& attr)
{
    const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
    if (!pAttr)
    {
        throw std::logic_error(std::string("Failed to get attribute from element: ") + attr);
    }
    
    return pAttr;
}

std::string Element::getAttributeOptional(const std::string& attr, const std::string& defaultValue)
{
    const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
    return pAttr ? pAttr : defaultValue;
}

NodeList Element::getElementsByTagName(const std::string& tagName)
{
    NodeList list = ixmlElement_getElementsByTagName(m_pElement, tagName.c_str());
    if (!list)
    {
        throw std::logic_error(std::string("Failed to get element subelements with tag: ") + tagName);
    }
    
    return list;
}

Element Element::getChildElement(const std::string& tagName)
{
    for (Element elem : getChildNodes())
    {
        if (elem.getName() == tagName)
        {
            return elem;
        }
    }
    
    throw std::logic_error(std::string("No child element found with name: " + tagName));
}

std::string Element::getChildElementValue(const std::string& tagName)
{
    return getChildElement(tagName).getValue();
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

std::vector<std::string> getActionsFromDescription(Document& doc)
{
    std::vector<std::string> actions;
    
    NodeList nodeList = doc.getElementsByTagName("action");
    uint64_t numActions = nodeList.size();
    actions.reserve(numActions);
    
    for (uint64_t i = 0; i < numActions; ++i)
    {
        Element actionElem = nodeList.getNode(i);
        actions.push_back(actionElem.getChildElementValue("name"));
    }
    
    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(Document& doc)
{
    std::vector<StateVariable> variables;
    
    NodeList nodeList = doc.getElementsByTagName("stateVariable");
    uint64_t numVariables = nodeList.size();
    variables.reserve(numVariables);
    
    for (uint64_t i = 0; i < numVariables; ++i)
    {
        Element elem = nodeList.getNode(i);
        
        try
        {
            StateVariable var;
            var.sendsEvents = elem.getAttribute("sendEvents") == "yes";
            var.name        = elem.getChildElementValue("name");
            var.dataType    = elem.getChildElementValue("dataType");
            
            try
            {
                Element rangeElement = elem.getElementsByTagName("allowedValueRange").getNode(0);
                std::unique_ptr<StateVariable::ValueRange> range(new StateVariable::ValueRange());
                
                range->minimumValue    = stringops::toNumeric<uint32_t>(rangeElement.getChildElementValue("minimum"));
                range->maximumValue    = stringops::toNumeric<uint32_t>(rangeElement.getChildElementValue("maximum"));
                range->step            = stringops::toNumeric<uint32_t>(rangeElement.getChildElementValue("step"));
                
                var.valueRange = std::move(range);
            }
            catch(std::exception&) { /* no value range for this element, no biggy */ }
            
            variables.push_back(var);
        }
        catch(std::exception& e)
        {
            log::warn("Failed to parse state variable, skipping:", e.what());
        }
    }
    
    return variables;
}

std::map<std::string, std::string> getEventValues(Document& doc)
{
    std::map<std::string, std::string> values;
    
    NodeList nodeList = doc.getElementsByTagName("InstanceID");
    NodeList children = nodeList.getNode(0).getChildNodes();
    
    uint64_t numVars = children.size();
    for (uint64_t i = 0; i < numVars; ++i)
    {
        Element elem = children.getNode(i);
        values.insert(std::make_pair(elem.getName(), elem.getAttribute("val")));
    }
    
    return values;
}

}
}
