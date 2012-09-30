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

class IXmlNode;

class IXmlNodeList
{
public:
    IXmlNodeList();
    IXmlNodeList(IXML_NodeList* pList);
    IXmlNodeList(const IXmlNodeList& list) = delete;
    IXmlNodeList(IXmlNodeList&& list);
    ~IXmlNodeList();
    
    IXmlNodeList& operator= (const IXmlNodeList& other) = delete;
    IXmlNodeList& operator= (IXML_NodeList* pList);
    
    operator IXML_NodeList*() const;
    operator bool() const;
    
    IXmlNode getNode(uint64_t index);
    uint64_t size();
    
private:
    IXML_NodeList*  m_pList;
};


class IXmlNamedNodeMap
{
public:
    IXmlNamedNodeMap();
    IXmlNamedNodeMap(IXML_NamedNodeMap* pNode);
    IXmlNamedNodeMap(const IXmlNamedNodeMap& node) = delete;
    IXmlNamedNodeMap(IXmlNamedNodeMap&& node);
    ~IXmlNamedNodeMap();
    
    uint64_t size() const;
    IXmlNode getNode(uint64_t index) const;
    
    operator IXML_NamedNodeMap*() const;
    operator bool() const;
    
private:
    IXML_NamedNodeMap*  m_pNodeMap;
};


class IXmlNode
{
public:
    IXmlNode();
    IXmlNode(IXML_Node* pNode);
    IXmlNode(const IXmlNode& node) = delete;
    IXmlNode(IXmlNode&& node);
    virtual ~IXmlNode();
    
    operator IXML_Node*() const;
    virtual operator bool() const;
    
    virtual std::string getName();
    virtual std::string getValue();
    virtual IXmlNamedNodeMap getAttributes();
    IXmlNode getParent();
    
    IXmlNode getFirstChild();
    IXmlNodeList getChildNodes();
    
    virtual std::string toString();

protected:
    void setNodePointer(IXML_Node* pNode);
    
private:
    IXML_Node*  m_pNode;
};

class IXmlDocument : public IXmlNode
{
public:
    enum OwnershipType
    {
        TakeOwnership,
        NoOwnership
    };

    IXmlDocument();
    IXmlDocument(const std::string& xml);
    IXmlDocument(IXML_Document* pDoc, OwnershipType ownership = TakeOwnership);
    IXmlDocument(const IXmlDocument& doc);
    IXmlDocument(IXmlDocument&& doc);
    ~IXmlDocument();

    IXmlDocument& operator= (const IXmlDocument& other) = delete;
    IXmlDocument& operator= (IXML_Document* pDoc);

    operator IXML_Document*() const;
    operator bool() const;
    IXML_Document** operator &();
    
    IXmlNodeList getElementsByTagName(const std::string& tagName);
    std::string getChildElementValue(const std::string& tagName);
    std::string getChildElementValueRecursive(const std::string& tagName);
    
    std::string toString() const;

private:
    IXML_Document*  m_pDoc;
    OwnershipType   m_Ownership;
};



class IXmlElement : public IXmlNode
{
public:
    IXmlElement();
    IXmlElement(IXML_Element* pElement);
    IXmlElement(IXmlNode&& node);
    IXmlElement(const IXmlElement& node) = delete;
    IXmlElement(IXmlElement&& node);
    
    IXmlElement& operator= (IXmlNode& node);
    
    operator IXML_Element*() const;
    operator bool() const;
    
    std::string getName();
    std::string getValue();
    std::string getAttribute(const std::string& attr);
    
    IXmlNodeList getElementsByTagName(const std::string& tagName);
    std::string getChildElementValue(const std::string& tagName);
    
private:
    IXML_Element*  m_pElement;
};

class IXmlString
{
public:
    IXmlString(DOMString str);
    IXmlString(const IXmlString& str) = delete;
    ~IXmlString();
    
    IXmlString& operator= (const IXmlString& other) = delete;
    
    operator std::string() const;
    operator DOMString() const;
    operator bool() const;
    
private:
    DOMString   m_String;
};

std::vector<StateVariable> getStateVariablesFromDescription(IXmlDocument& doc);
std::vector<std::string> getActionsFromDescription(IXmlDocument& doc);
std::map<std::string, std::string> getEventValues(IXmlDocument& doc);

}

#endif
