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

#include "upnp/upnpitem.h"


namespace upnp
{

static const std::string protocolInfo   = "protocolInfo";
static const std::string size           = "size";
static const std::string resolution     = "resolution";
static const std::string dlnaThumbnail  = "DLNA.ORG_PN=JPEG_TN";

static const std::string emptyString;
static const std::string titleTag       = "dc:title";
static const std::string classTag       = "upnp:class";
static const std::string resourceTag    = "res";

Resource::Resource()
{
}

Resource::Resource(const Resource& other)
: m_MetaData(other.m_MetaData)
, m_Url(other.m_Url)
{
}

Resource::Resource(Resource&& other)
: m_MetaData(std::move(other.m_MetaData))
, m_Url(std::move(other.m_Url))
{    
}

Resource& Resource::operator=(const Resource& other)
{
    m_MetaData = other.m_MetaData;
    m_Url = other.m_Url;
    
    return *this;
}

Resource& Resource::operator=(Resource&& other)
{
    m_MetaData = std::move(other.m_MetaData);
    m_Url = std::move(other.m_Url);
    
    return *this;
}

const std::string& Resource::getMetaString(MetaData meta)
{
    switch (meta)
    {
    case ProtocolInfo:
        return protocolInfo;
    case Size:
        return size;
    case Resolution:
        return resolution;
    }
    
    return emptyString;
}

const std::string& Resource::getMetaData(MetaData meta) const
{
    const std::string& metaString = getMetaString(meta);
    MetaMap::const_iterator iter = m_MetaData.find(metaString);
    if (iter != m_MetaData.end())
    {
        return iter->second;
    }
    
    return emptyString;
}

const std::string& Resource::getUrl() const
{
    return m_Url;
}

bool Resource::isThumbnail() const
{
    return (0 != getMetaData(ProtocolInfo).find(dlnaThumbnail));
}

void Resource::setUrl(const std::string& url)
{
    m_Url = url;
}

void Resource::addMetaData(const std::string& key, const std::string& value)
{
    m_MetaData[key] = value;
}

Item::Item(const std::string& id, const std::string& title)
: m_ObjectId(id)
, m_ChildCount(0)
{
    setTitle(title);
}

Item::Item(const Item& other)
: m_ObjectId(other.m_ObjectId)
, m_ParentId(other.m_ParentId)
, m_MetaData(other.m_MetaData)
, m_Resources(other.m_Resources)
, m_ChildCount(other.m_ChildCount)
{
}

Item::Item(Item&& other)
: m_ObjectId(std::move(other.m_ObjectId))
, m_ParentId(std::move(other.m_ParentId))
, m_MetaData(std::move(other.m_MetaData))
, m_Resources(std::move(other.m_Resources))
, m_ChildCount(other.m_ChildCount)
{
}

Item::~Item()
{
}

Item& Item::operator= (const Item& other)
{
    m_ObjectId      = other.m_ObjectId;
    m_ParentId      = other.m_ParentId;
    m_MetaData      = other.m_MetaData;
    m_Resources     = other.m_Resources;
    
    m_ChildCount    = other.m_ChildCount;    

    return *this;
}

Item& Item::operator= (Item&& other)
{
    m_ObjectId      = std::move(other.m_ObjectId);
    m_ParentId      = std::move(other.m_ParentId);
    m_MetaData      = std::move(other.m_MetaData);
    m_Resources     = std::move(other.m_Resources);
    
    m_ChildCount    = other.m_ChildCount;
    
    return *this;
}

const std::string& Item::getObjectId() const
{
    return m_ObjectId;
}

const std::string& Item::getParentId() const
{
    return m_ParentId;
}

const std::string& Item::getTitle() const
{
    MetaMap::const_iterator iter = m_MetaData.find(titleTag);
    if (iter != m_MetaData.end())
    {
        return iter->second;
    }
    
    return emptyString;
}

const std::vector<Resource>& Item::getResources() const
{
    return m_Resources;
}

uint32_t Item::getChildCount() const
{
    return m_ChildCount;
}

Item::Class Item::getClass() const
{
    MetaMap::const_iterator iter = m_MetaData.find(classTag);
    if (iter == m_MetaData.end())
    {
        return Unknown;
    }

    const std::string& upnpClass = iter->second;
    if (0 == upnpClass.find("object.item.audioItem"))
    {
        return Audio;
    }
    else if (0 == upnpClass.find("object.item.imageItem"))
    {
        return Image;
    }
    else if (0 == upnpClass.find("object.item.videoItem"))
    {
        return Video;
    }
    else if (upnpClass == "object.item")
    {
        return Generic;
    }
    else if (upnpClass == "object.container.videoContainer")
    {
        return VideoContainer;
    }
    else if (upnpClass == "object.container.album.musicAlbum")
    {
        return AudioContainer;
    }
    else if (upnpClass == "object.container.album.photoAlbum")
    {
        return ImageContainer;
    }
    else if (0 == upnpClass.find("object.container"))
    {
        return Container;
    }
    
    return Unknown;
}

void Item::setObjectId(const std::string& id)
{
    m_ObjectId = id;
}

void Item::setParentId(const std::string& id)
{
    m_ParentId = id;
}

void Item::setTitle(const std::string& title)
{
    m_MetaData[titleTag] = title;
}

void Item::setChildCount(uint32_t count)
{
    m_ChildCount = count;
}

void Item::addMetaData(const std::string& key, const std::string& value)
{
    m_MetaData[key] = value;
}

void Item::addResource(const Resource& resource)
{
    m_Resources.push_back(resource);
}

std::string Item::getMetaData(const std::string& key) const
{
    auto iter = m_MetaData.find(key);
    if (iter != m_MetaData.end())
    {
        return iter->second;
    }
    
    return "";
}

}
