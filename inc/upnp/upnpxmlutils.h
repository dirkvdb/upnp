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

#ifndef UPNP_XML_UTILS_H
#define UPNP_XML_UTILS_H

#include <map>
#include <string>
#include <vector>
#include <cassert>

#include <upnp/upnp.h>

#include "upnp/upnpstatevariable.h"
#include "utils/stringoperations.h"

namespace upnp
{
namespace xml
{

class Document;
class NodeList;
class NamedNodeMap;

class String
{
public:
    String(DOMString str);
    String(const String& str) = delete;
    ~String();
    
    String& operator= (const String& other) = delete;
    
    operator std::string() const;
    operator DOMString() const;
    operator bool() const;
    
private:
    DOMString   m_String;
};

template <class Container, class Type>
class Iterator
{
public:
    Iterator(const Container& container, uint64_t index)
    : m_Container(container)
    , m_Index(index)
    {}
    
    Iterator& operator ++()
    {
        ++m_Index;
        return *this;
    }
    
    Iterator& operator --()
    {
        assert(m_Index > 0);
        
        --m_Index;
        return *this;
    }
    
    bool operator ==(const Iterator& other) const
    {
        return m_Index == other.m_Index;
    }
    
    bool operator !=(const Iterator& other) const
    {
        return m_Index != other.m_Index;
    }
    
    const Type operator *()
    {
        return m_Container[m_Index];
    }
    
private:
    const Container&    m_Container;
    uint64_t            m_Index;
};

class Node
{
public:
    Node();
    Node(IXML_Node* pNode);
    Node(const Node& node);
    Node(Node&& node) = default;
    virtual ~Node();
    
    operator IXML_Node*() const;
    virtual operator bool() const;
    Node& operator= (Node&& other) = default;
    bool operator == (const Node& other) const;
    bool operator != (const Node& other) const;
    
    virtual std::string getName() const;
    virtual std::string getValue() const;
    virtual NamedNodeMap getAttributes() const;
    Node getParent() const;
    
    Node getFirstChild() const;
    NodeList getChildNodes() const;
    Node getChildNode(const std::string& tagName) const;
    std::string getChildNodeValue(const std::string& tagName) const;
    Document getOwnerDocument() const;
    
    void appendChild(Node& node);
    
    virtual std::string toString() const;

protected:
    void setNodePointer(IXML_Node* pNode);
    
private:
    IXML_Node*  m_pNode;
};

class Element : public Node
{
public:
    Element();
    Element(IXML_Element* pElement);
    Element(const Node& node);
    Element(Node&& node);
    Element(const Element& node) = delete;
    Element(Element&& node) = default;
    
    Element& operator= (Node& node);
    
    operator IXML_Element*() const;
    operator bool() const;
    
    virtual std::string getName() const;
    virtual std::string getValue() const;
    std::string getAttribute(const std::string& attr);
    std::string getAttributeOptional(const std::string& attr, const std::string& defaultValue = "");
    void addAttribute(const std::string& name, const std::string& value);
    
    template <typename T>
    T getAttributeAsNumeric(const std::string& attr)
    {
        return utils::stringops::toNumeric<T>(getAttribute(attr));
    }
    
    template <typename T>
    T getAttributeAsNumericOptional(const std::string& attr, T defaultValue)
    {
        const char* pAttr = ixmlElement_getAttribute(m_pElement, attr.c_str());
        return pAttr ? utils::stringops::toNumeric<T>(pAttr) : defaultValue;
    }
    
    NodeList getElementsByTagName(const std::string& tagName);
    Element getChildElement(const std::string& tagName);
    
private:
    IXML_Element*  m_pElement;
};

class Document : public Node
{
public:
    enum OwnershipType
    {
        TakeOwnership,
        NoOwnership
    };

    Document();
    Document(const std::string& xml);
    Document(IXML_Document* pDoc, OwnershipType ownership = TakeOwnership);
    Document(const Document& doc);
    Document(Document&& doc);
    ~Document();

    Document& operator= (const Document& other) = delete;
    Document& operator= (Document&& other);
    Document& operator= (IXML_Document* pDoc);

    operator IXML_Document*() const;
    operator bool() const;
    
    NodeList getElementsByTagName(const std::string& tagName) const;
    std::string getChildNodeValueRecursive(const std::string& tagName) const;
    
    Node createNode(const std::string& value);
    Element createElement(const std::string& name);
    Element createElementNamespaced(const std::string& nameSpace, const std::string& name);
    
    virtual std::string toString() const;

private:
    IXML_Document*  m_pDoc;
    OwnershipType   m_Ownership;
};

class NodeList
{
public:
    NodeList();
    NodeList(IXML_NodeList* pList);
    NodeList(const NodeList& list) = delete;
    NodeList(NodeList&& list) = default;
    ~NodeList();
    
    NodeList& operator= (const NodeList& other) = delete;
    NodeList& operator= (IXML_NodeList* pList);
    Node operator[] (uint64_t index) const;
    
    operator IXML_NodeList*() const;
    operator bool() const;
    
    Node getNode(uint64_t index) const;
    uint64_t size() const;
    
private:
    IXML_NodeList*  m_pList;
};

class NamedNodeMap
{
public:
    NamedNodeMap();
    NamedNodeMap(IXML_NamedNodeMap* pNode);
    NamedNodeMap(const NamedNodeMap& node) = delete;
    NamedNodeMap(NamedNodeMap&& node) = default;
    ~NamedNodeMap();
    
    uint64_t size() const;
    Node getNode(uint64_t index) const;
    
    operator IXML_NamedNodeMap*() const;
    operator bool() const;
    Node operator[] (uint64_t index) const;
    
private:
    IXML_NamedNodeMap*  m_pNodeMap;
};

// support for range for loops
template <typename Iterable>
inline Iterator<Iterable, Node> begin(const Iterable& iterable)
{
    return Iterator<Iterable, Node>(iterable, 0);
}

template <typename Iterable>
inline Iterator<Iterable, Node> end(const Iterable& iterable)
{
    return Iterator<Iterable, Node>(iterable, iterable.size());
}


std::vector<StateVariable> getStateVariablesFromDescription(Document& doc);
std::vector<std::string> getActionsFromDescription(Document& doc);
std::map<std::string, std::string> getEventValues(Document& doc);

}
}

#endif
