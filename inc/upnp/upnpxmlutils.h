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

#include <upnp/upnp.h>

#include "upnp/upnpstatevariable.h"

namespace upnp
{
namespace xml
{

class NodeList;
class NamedNodeMap;

class Node
{
public:
    Node();
    Node(IXML_Node* pNode);
    Node(const Node& node) = delete;
    Node(Node&& node);
    virtual ~Node();
    
    operator IXML_Node*() const;
    virtual operator bool() const;
    
    virtual std::string getName() const;
    virtual std::string getValue() const;
    virtual NamedNodeMap getAttributes() const;
    Node getParent() const;
    
    Node getFirstChild() const;
    NodeList getChildNodes() const;
    
    virtual std::string toString();

protected:
    void setNodePointer(IXML_Node* pNode);
    
private:
    IXML_Node*  m_pNode;
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
    Document& operator= (IXML_Document* pDoc);

    operator IXML_Document*() const;
    operator bool() const;
    IXML_Document** operator &();
    
    NodeList getElementsByTagName(const std::string& tagName) const;
    std::string getChildElementValue(const std::string& tagName) const;
    std::string getChildElementValueRecursive(const std::string& tagName) const;
    
    std::string toString() const;

private:
    IXML_Document*  m_pDoc;
    OwnershipType   m_Ownership;
};

class Element : public Node
{
public:
    Element();
    Element(IXML_Element* pElement);
    Element(Node&& node);
    Element(const Element& node) = delete;
    Element(Element&& node);
    
    Element& operator= (Node& node);
    
    operator IXML_Element*() const;
    operator bool() const;
    
    std::string getName();
    std::string getValue();
    std::string getAttribute(const std::string& attr);
    
    NodeList getElementsByTagName(const std::string& tagName);
    std::string getChildElementValue(const std::string& tagName);
    
private:
    IXML_Element*  m_pElement;
};


class NodeList
{
public:
    NodeList();
    NodeList(IXML_NodeList* pList);
    NodeList(const NodeList& list) = delete;
    NodeList(NodeList&& list);
    ~NodeList();
    
    NodeList& operator= (const NodeList& other) = delete;
    NodeList& operator= (IXML_NodeList* pList);
    
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
    NamedNodeMap(NamedNodeMap&& node);
    ~NamedNodeMap();
    
    uint64_t size() const;
    Node getNode(uint64_t index) const;
    
    operator IXML_NamedNodeMap*() const;
    operator bool() const;
    
private:
    IXML_NamedNodeMap*  m_pNodeMap;
};


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

std::vector<StateVariable> getStateVariablesFromDescription(Document& doc);
std::vector<std::string> getActionsFromDescription(Document& doc);
std::map<std::string, std::string> getEventValues(Document& doc);

}
}

#endif
