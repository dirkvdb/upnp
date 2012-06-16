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

namespace upnp
{

class IXmlDocument
{
public:
    IXmlDocument();
    IXmlDocument(IXML_Document* pDoc);
    IXmlDocument(const IXmlDocument& doc) = delete;
    IXmlDocument(IXmlDocument&& doc);
    ~IXmlDocument();

    IXmlDocument& operator= (const IXmlDocument& other) = delete;
    IXmlDocument& operator= (IXML_Document* pDoc);

    operator IXML_Document*() const;
    operator bool() const;
    IXML_Document** operator &();

private:
    IXML_Document*  m_pDoc;
};

class IXmlNodeList
{
public:
    IXmlNodeList();
    IXmlNodeList(IXML_NodeList* pList);
    IXmlNodeList(const IXmlNodeList& doc) = delete;
    IXmlNodeList(IXmlNodeList&& list);
    ~IXmlNodeList();

    IXmlNodeList& operator= (const IXmlNodeList& other) = delete;
    IXmlNodeList& operator= (IXML_NodeList* pList);

    operator IXML_NodeList*() const;
    operator bool() const;

private:
    IXML_NodeList*  m_pList;
};

class IXmlString
{
public:
    IXmlString(DOMString str);
    IXmlString(const IXmlString& str) = delete;
    ~IXmlString();
    
    IXmlString& operator= (const IXmlString& other) = delete;
    
    operator DOMString() const;
    operator bool() const;
    
private:
    DOMString   m_String;
};

std::string getFirstElementValue(IXmlNodeList& nodeList, const std::string& item);
std::string getFirstElementValue(IXmlDocument& doc, const std::string& item);
std::string getFirstElementValue(IXML_Element* pElement, const std::string& item);
std::vector<std::string> getActionsFromDescription(IXmlDocument& doc);
std::map<std::string, std::string> getEventValues(IXmlDocument& doc);

}

#endif
