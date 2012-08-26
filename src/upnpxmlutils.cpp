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

IXmlDocument::IXmlDocument()
: m_pDoc(nullptr)
{
}

IXmlDocument::IXmlDocument(IXML_Document* pDoc)
: m_pDoc(pDoc)
{
}

IXmlDocument::IXmlDocument(IXmlDocument&& doc)
: m_pDoc(doc.m_pDoc)
{
    doc.m_pDoc = nullptr;
}

IXmlDocument::~IXmlDocument()
{
    ixmlDocument_free(m_pDoc);
}

IXmlDocument& IXmlDocument::operator= (IXML_Document* pDoc)
{
    m_pDoc = pDoc;
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

IXmlNodeList::IXmlNodeList()
: m_pList(nullptr)
{
}

IXmlNodeList::IXmlNodeList(IXML_NodeList* pList)
: m_pList(pList)
{
}

IXmlNodeList::IXmlNodeList(IXmlNodeList&& doc)
: m_pList(doc.m_pList)
{
    doc.m_pList = nullptr;
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

IXmlNode IXmlNodeList::getNode(unsigned long index)
{
    IXmlNode node = ixmlNodeList_item(m_pList, index);
    if (!node)
    {
        throw std::logic_error(std::string("Failed to find node in nodelist on index: ") + numericops::toString(index));
    }
    
    return node;
}

unsigned long IXmlNodeList::getLength()
{
    return ixmlNodeList_length(m_pList);
}

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

IXmlElement::IXmlElement()
: m_pElement(nullptr)
{
}

IXmlElement::IXmlElement(IXML_Element* pElement)
: m_pElement(pElement)
{
}

IXmlElement::IXmlElement(IXmlNode&& node)
: m_pElement(std::move(reinterpret_cast<IXML_Element*>(static_cast<IXML_Node*>(node))))
{
}

IXmlElement::IXmlElement(IXmlElement&& node)
: m_pElement(std::move(node.m_pElement))
{
}

IXmlElement& IXmlElement::operator= (IXmlNode& node)
{
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

IXmlString::IXmlString(DOMString str)
: m_String(str)
{
}

IXmlString::~IXmlString()
{
    ixmlFreeDOMString(m_String);
}

IXmlString::operator DOMString() const
{
    return m_String;
}

IXmlString::operator bool() const
{
    return m_String != nullptr;
}

std::string getFirstElementAttribute(IXmlNodeList& nodeList, const std::string& item, const std::string& attribute)
{
    std::string result;
    
    IXML_Element* pTmpNode = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(nodeList, 0));
    if (pTmpNode == nullptr)
    {
        throw std::logic_error("Failed to find XML element value: " + item);
    }
    
    const char* pValue = ixmlElement_getAttribute(pTmpNode, attribute.c_str());
    if (pValue)
    {
        result = pValue;
    }
    
    return result;
}

std::string getFirstElementAttribute(IXmlDocument& doc, const std::string& item, const std::string& attribute)
{
    IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(doc, item.c_str());
    if (!nodeList)
    {
        throw std::logic_error("Failed to find element value in document: " + item);
    }
    
    return getFirstElementAttribute(nodeList, item, attribute);
}

std::string getFirstElementValue(IXmlNodeList& nodeList, const std::string& item)
{
    std::string result;
    
    IXML_Node* pTmpNode = ixmlNodeList_item(nodeList, 0);
    if (pTmpNode == nullptr)
    {
        throw std::logic_error("Failed to find XML element value: " + item);
    }
    
    IXML_Node* pTextNode = ixmlNode_getFirstChild(pTmpNode);
    const char* pValue = ixmlNode_getNodeValue(pTextNode);
    if (pValue)
    {
        result = pValue;
    }
    
    return result;
}

std::string getFirstElementValue(IXmlDocument& doc, const std::string& item)
{
    IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(doc, item.c_str());
    if (!nodeList)
    {
        throw std::logic_error("Failed to find element value in document: " + item);
    }

    return getFirstElementValue(nodeList, item);
}

std::string getFirstElementValue(IXML_Element* pElement, const std::string& item)
{
    std::string result;
    
    IXmlNodeList nodeList = ixmlElement_getElementsByTagName(pElement, item.c_str());
    if (!nodeList)
    {
        return result;
    }
    
    return getFirstElementValue(nodeList, item);
}

std::vector<std::string> getActionsFromDescription(IXmlDocument& doc)
{
    std::vector<std::string> actions;
    
    IXmlNodeList nodeList = ixmlDocument_getElementsByTagName(doc, "action");
    if (!nodeList)
    {
        throw std::logic_error("Failed to find actions in document");
    }
    
    unsigned long numActions = ixmlNodeList_length(nodeList);
    actions.reserve(numActions);
    
    for (unsigned long i = 0; i < numActions; ++i)
    {
        IXML_Element* pActionElem = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(nodeList, i));
        if (!pActionElem)
        {
            log::error("Failed to get action from action list, skipping");
            continue;
        }
    
        actions.push_back(getFirstElementValue(pActionElem, "name"));
    }
    
    return actions;
}

std::vector<StateVariable> getStateVariablesFromDescription(IXmlDocument& doc)
{
    std::vector<StateVariable> variables;
    
    IXmlNodeList nodeList = doc.getElementsByTagName("stateVariable");
    unsigned long numVariables = nodeList.getLength();
    variables.reserve(numVariables);
    
    for (unsigned long i = 0; i < numVariables; ++i)
    {
        IXmlElement elem = nodeList.getNode(i);
        
        try
        {
            StateVariable var;
            var.sendsEvents = elem.getAttribute("sendEvents") == "yes";
            
            var.name        = elem.getElementsByTagName("name").getNode(0).getFirstChild().getValue();
            var.dataType    = elem.getElementsByTagName("dataType").getNode(0).getFirstChild().getValue();
            
            try
            {
                IXmlElement rangeElement = elem.getElementsByTagName("allowedValueRange").getNode(0);
                var.valueRange.reset(new StateVariable::ValueRange());
                IXmlNode minNode = rangeElement.getElementsByTagName("minimum").getNode(0);
                IXmlNode maxNode = rangeElement.getElementsByTagName("maximum").getNode(0);
                IXmlNode stepNode = rangeElement.getElementsByTagName("step").getNode(0);
                
                var.valueRange->minimumValue    = stringops::toNumeric<uint32_t>(minNode.getFirstChild().getValue());
                var.valueRange->maximumValue    = stringops::toNumeric<uint32_t>(maxNode.getFirstChild().getValue());
                var.valueRange->step            = stringops::toNumeric<uint32_t>(stepNode.getFirstChild().getValue());
            }
            catch(std::exception&)
            {
                // no value range for this element, no biggy
            }
            
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
    IXmlNode instanceNode = nodeList.getNode(0);
    IXmlNodeList children = instanceNode.getChildNodes();
    
    unsigned long numVars = children.getLength();
    for (unsigned long i = 0; i < numVars; ++i)
    {
        IXmlElement elem = children.getNode(i);
        values.insert(std::make_pair(elem.getName(), elem.getAttribute("val")));
    }
    
    return values;
}

}
