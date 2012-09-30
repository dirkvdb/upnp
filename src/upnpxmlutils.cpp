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

IXmlNode::IXmlNode()
: m_pNode(nullptr)
{
}

IXmlNode::IXmlNode(IXML_Node* pNode)
: m_pNode(pNode)
{
}

IXmlNode::IXmlNode(IXmlNode&& node)
: m_pNode(std::move(node.m_pNode))
{
}

IXmlNode::~IXmlNode()
{
}

IXmlNode::operator IXML_Node*() const
{
    return m_pNode;
}

IXmlNode::operator bool() const
{
    return m_pNode != nullptr;
}

std::string IXmlNode::getName()
{
    return ixmlNode_getNodeName(m_pNode);
}

std::string IXmlNode::getValue()
{
    const char* pStr = ixmlNode_getNodeValue(m_pNode);
    if (!pStr)
    {
        throw std::logic_error(std::string("Failed to get node value: ") + getName());
    }
    
    return pStr;
}

std::string IXmlNode::toString()
{
    IXmlString str(ixmlPrintNode(m_pNode));
    if (!str)
    {
        throw std::logic_error("Failed to convert node to string");
    }
    
    return str;
}

void IXmlNode::setNodePointer(IXML_Node* pNode)
{
    m_pNode = pNode;
}

IXmlDocument::IXmlDocument()
: m_pDoc(nullptr)
, m_Ownership(NoOwnership)
{
}

IXmlDocument::IXmlDocument(const std::string& xml)
: m_pDoc(ixmlParseBuffer(xml.c_str()))
, m_Ownership(TakeOwnership)
{
    if (!m_pDoc)
    {
        throw std::logic_error("Invalid xml document string received");
    }
    
    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
}

IXmlDocument::IXmlDocument(IXML_Document* pDoc, OwnershipType ownership)
: IXmlNode(reinterpret_cast<IXML_Node*>(pDoc))
, m_pDoc(pDoc)
, m_Ownership(ownership)
{
}

IXmlDocument::IXmlDocument(const IXmlDocument& doc)
: m_pDoc(nullptr)
, m_Ownership(TakeOwnership)
{
    // this copy constructor is only meant to be used in unit tests where the use of gmock requries copy constructors
    // real code sjould avoid copies of the document for performance reasons

    if (doc)
    {
        //m_pDoc = doc.m_pDoc;
        //setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
    
        //log::warn("IXmlDocument copy constructor is implmented for gmock compatibility, should not get executed in code for performance reaseons");
        
        m_pDoc = ixmlDocument_createDocument();
        setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
        
        IXmlNodeList nodeList = ixmlNode_getChildNodes(reinterpret_cast<IXML_Node*>(static_cast<IXML_Document*>(doc)));
        for (int i = 0; i < nodeList.size(); ++i)
        {
            IXML_Node* pNode = nullptr;
            IXmlNode node = nodeList.getNode(i);
            if (IXML_SUCCESS != ixmlDocument_importNode(m_pDoc, node, TRUE, &pNode))
            {
                throw std::logic_error("Failed to clone xml document");
            }
            
            ixmlNode_appendChild(reinterpret_cast<IXML_Node*>(static_cast<IXML_Document*>(m_pDoc)), pNode);
        }
    }
}

IXmlDocument::IXmlDocument(IXmlDocument&& doc)
: IXmlNode(std::forward<IXmlNode>(doc))
, m_pDoc(std::move(doc.m_pDoc))
, m_Ownership(TakeOwnership)
{
}

IXmlDocument::~IXmlDocument()
{
    if (TakeOwnership == m_Ownership)
    {
        //ixmlDocument_free(m_pDoc);
    }
}

IXmlDocument& IXmlDocument::operator= (IXML_Document* pDoc)
{
    m_pDoc = pDoc;
    setNodePointer(reinterpret_cast<IXML_Node*>(m_pDoc));
    return *this;
}

IXML_Document** IXmlDocument::operator &()
{
    return &m_pDoc;
}

IXmlDocument::operator IXML_Document*() const
{
    return m_pDoc;
}

IXmlDocument::operator bool() const
{
    return m_pDoc != nullptr;
}

IXmlNodeList IXmlDocument::getElementsByTagName(const std::string& tagName)
{
    IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(m_pDoc, tagName.c_str());
    if (!nodeList)
    {
        throw std::logic_error(std::string("Failed to find tags in document with name: ") + tagName);
    }
    
    return nodeList;
}

std::string IXmlDocument::getChildElementValue(const std::string& tagName)
{
    IXmlNodeList list = getChildNodes();
    for (uint64_t i = 0; i < list.size(); ++i)
    {
        IXmlNode node = list.getNode(i);
        if (node.getName() == tagName)
        {
            return node.getFirstChild().getValue();
        }
    }
    
    throw std::logic_error(std::string("No child element found in document with name ") + tagName);
}

std::string IXmlDocument::getChildElementValueRecursive(const std::string& tagName)
{
    IXmlNodeList nodeList = getElementsByTagName(tagName);
    if (nodeList.size() == 0)
    {
        throw std::logic_error(std::string("Failed to get document subelement value with tag: ") + tagName);
    }
    
    return nodeList.getNode(0).getFirstChild().getValue();
}

std::string IXmlDocument::toString() const
{
    IXmlString str(ixmlDocumenttoString(m_pDoc));
    if (!str)
    {
        throw std::logic_error("Failed to convert document to string");
    }
    
    return str;
}

IXmlNodeList::IXmlNodeList()
: m_pList(nullptr)
{
}

IXmlNodeList::IXmlNodeList(IXML_NodeList* pList)
: m_pList(pList)
{
}

IXmlNodeList::IXmlNodeList(IXmlNodeList&& doc)
: m_pList(std::move(doc.m_pList))
{
}

IXmlNodeList::~IXmlNodeList()
{
    ixmlNodeList_free(m_pList);
}

IXmlNodeList& IXmlNodeList::operator= (IXML_NodeList* pList)
{
    m_pList = pList;
    return *this;
}

IXmlNodeList::operator IXML_NodeList*() const
{
    return m_pList;
}

IXmlNodeList::operator bool() const
{
    return m_pList != nullptr;
}

IXmlNode IXmlNodeList::getNode(uint64_t index)
{
    IXmlNode node = ixmlNodeList_item(m_pList, index);
    if (!node)
    {
        throw std::logic_error(std::string("Failed to find node in nodelist on index: ") + numericops::toString(index));
    }
    
    return node;
}

uint64_t IXmlNodeList::size()
{
    return ixmlNodeList_length(m_pList);
}

IXmlNamedNodeMap IXmlNode::getAttributes()
{
    IXmlNamedNodeMap nodeMap = ixmlNode_getAttributes(m_pNode);
    if (!nodeMap)
    {
        throw std::logic_error(std::string("Failed to get attribute map from node: ") + getName());
    }
    
    return nodeMap;
}

IXmlNode IXmlNode::getParent()
{
    return ixmlNode_getParentNode(m_pNode);
}

IXmlNode IXmlNode::getFirstChild()
{
    IXmlNode node = ixmlNode_getFirstChild(m_pNode);
    if (!node)
    {
        throw std::logic_error(std::string("Failed to get first child node from node: ") + getName());
    }
    
    return node;
}

IXmlNodeList IXmlNode::getChildNodes()
{
    IXmlNodeList children = ixmlNode_getChildNodes(m_pNode);
    if (!children)
    {
        throw std::logic_error(std::string("Failed to get childNodes from node: ") + getName());
    }
    
    return children;
}

IXmlNamedNodeMap::IXmlNamedNodeMap()
: m_pNodeMap(nullptr)
{
}

IXmlNamedNodeMap::IXmlNamedNodeMap(IXML_NamedNodeMap* pNodeMap)
: m_pNodeMap(pNodeMap)
{
}

IXmlNamedNodeMap::IXmlNamedNodeMap(IXmlNamedNodeMap&& nodeMap)
: m_pNodeMap(std::move(nodeMap.m_pNodeMap))
{
}

IXmlNamedNodeMap::~IXmlNamedNodeMap()
{
    ixmlNamedNodeMap_free(m_pNodeMap);
}

uint64_t IXmlNamedNodeMap::size() const
{
    return ixmlNamedNodeMap_getLength(m_pNodeMap);
}

IXmlNode IXmlNamedNodeMap::getNode(uint64_t index) const
{
    IXmlNode node = ixmlNamedNodeMap_item(m_pNodeMap, index);
    if (!node)
    {
        throw std::logic_error("Failed to get node from named node map");
    }
    
    return node;
}

IXmlNamedNodeMap::operator IXML_NamedNodeMap*() const
{
    return m_pNodeMap;
}

IXmlNamedNodeMap::operator bool() const
{
    return m_pNodeMap != nullptr;
}

IXmlElement::IXmlElement()
: m_pElement(nullptr)
{
}

IXmlElement::IXmlElement(IXML_Element* pElement)
: IXmlNode(reinterpret_cast<IXML_Node*>(pElement))
, m_pElement(pElement)
{
}

// this move constructor allows a move of an XmlNode to an XmlElement, it upcasts the node
IXmlElement::IXmlElement(IXmlNode&& node)
: IXmlNode(std::forward<IXmlNode>(node))
, m_pElement(reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(*this)))
{
}

IXmlElement::IXmlElement(IXmlElement&& node)
: m_pElement(std::move(node.m_pElement))
{
}

// this assignment operator allows a copy of an XmlNode to an XmlElement, it upcasts the node
IXmlElement& IXmlElement::operator= (IXmlNode& node)
{
    setNodePointer(static_cast<IXML_Node*>(node));
    m_pElement = reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(node));
    return *this;
}

IXmlElement::operator IXML_Element*() const
{
    return m_pElement;
}

IXmlElement::operator bool() const
{
    return m_pElement != nullptr;
}

std::string IXmlElement::getName()
{
    return ixmlElement_getTagName(m_pElement);
}

std::string IXmlElement::getValue()
{
    return getFirstChild().getValue();
}

std::string IXmlElement::getAttribute(const std::string& attr)
{
    const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
    if (!pAttr)
    {
        throw std::logic_error(std::string("Failed to get attribute from element: ") + attr);
    }
    
    return pAttr;
}

IXmlNodeList IXmlElement::getElementsByTagName(const std::string& tagName)
{
    IXmlNodeList list = ixmlElement_getElementsByTagName(m_pElement, tagName.c_str());
    if (!list)
    {
        throw std::logic_error(std::string("Failed to get element subelements with tag: ") + tagName);
    }
    
    return list;
}

std::string IXmlElement::getChildElementValue(const std::string& tagName)
{
    IXmlNodeList list = getChildNodes();
    uint64_t size = list.size();
    for (uint64_t i = 0; i < size; ++i)
    {
        IXmlNode node = list.getNode(i);
        if (node.getName() == tagName)
        {
            return node.getFirstChild().getValue();
        }
    }
    
    throw std::logic_error(std::string("No child element found with name ") + tagName);
}

IXmlString::IXmlString(DOMString str)
: m_String(str)
{
}

IXmlString::~IXmlString()
{
    ixmlFreeDOMString(m_String);
}

IXmlString::operator std::string() const
{
    return std::string(m_String);
}

IXmlString::operator DOMString() const
{
    return m_String;
}

IXmlString::operator bool() const
{
    return m_String != nullptr;
}

std::vector<std::string> getActionsFromDescription(IXmlDocument& doc)
{
    std::vector<std::string> actions;
    
    IXmlNodeList nodeList = doc.getElementsByTagName("action");
    unsigned long numActions = nodeList.size();
    actions.reserve(numActions);
    
    for (unsigned long i = 0; i < numActions; ++i)
    {
        IXmlElement actionElem = nodeList.getNode(i);
        actions.push_back(actionElem.getChildElementValue("name"));
    }
    
    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(IXmlDocument& doc)
{
    std::vector<StateVariable> variables;
    
    IXmlNodeList nodeList = doc.getElementsByTagName("stateVariable");
    unsigned long numVariables = nodeList.size();
    variables.reserve(numVariables);
    
    for (unsigned long i = 0; i < numVariables; ++i)
    {
        IXmlElement elem = nodeList.getNode(i);
        
        try
        {
            StateVariable var;
            var.sendsEvents = elem.getAttribute("sendEvents") == "yes";
            var.name        = elem.getChildElementValue("name");
            var.dataType    = elem.getChildElementValue("dataType");
            
            try
            {
                IXmlElement rangeElement = elem.getElementsByTagName("allowedValueRange").getNode(0);
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

std::map<std::string, std::string> getEventValues(IXmlDocument& doc)
{
    std::map<std::string, std::string> values;
    
    IXmlNodeList nodeList = doc.getElementsByTagName("InstanceID");
    IXmlNodeList children = nodeList.getNode(0).getChildNodes();
    
    unsigned long numVars = children.size();
    for (unsigned long i = 0; i < numVars; ++i)
    {
        IXmlElement elem = children.getNode(i);
        values.insert(std::make_pair(elem.getName(), elem.getAttribute("val")));
    }
    
    return values;
}

}
