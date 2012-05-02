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

#include <stdexcept>

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
        throw std::logic_error("Failed to find element value in element: " + item);
    }
    
    return getFirstElementValue(nodeList, item);
}

}
