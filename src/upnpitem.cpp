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

#include "utils/log.h"

namespace upnp
{

static const std::string protocolInfo   = "protocolInfo";
static const std::string dlnaThumbnail  = "DLNA.ORG_PN=JPEG_TN";

static const std::string emptyString;
static const std::string resourceTag    = "res";

Resource::Resource()
{
}

Resource::Resource(const Resource& other)
: m_MetaData(other.m_MetaData)
, m_Url(other.m_Url)
, m_ProtocolInfo(other.m_ProtocolInfo)
{
}

Resource::Resource(Resource&& other)
: m_MetaData(std::move(other.m_MetaData))
, m_Url(std::move(other.m_Url))
, m_ProtocolInfo(std::move(other.m_ProtocolInfo))
{    
}

Resource& Resource::operator=(const Resource& other)
{
    m_MetaData      = other.m_MetaData;
    m_Url           = other.m_Url;
    m_ProtocolInfo  = other.m_ProtocolInfo;
        
    return *this;
}

Resource& Resource::operator=(Resource&& other)
{
    m_MetaData      = std::move(other.m_MetaData);
    m_Url           = std::move(other.m_Url);
    m_ProtocolInfo  = std::move(other.m_ProtocolInfo);
    
    return *this;
}

const std::string& Resource::getMetaData(const std::string& metaKey) const
{
    auto iter = m_MetaData.find(metaKey);
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

const ProtocolInfo& Resource::getProtocolInfo() const
{
    return m_ProtocolInfo;
}

bool Resource::isThumbnail() const
{
    return (0 != getMetaData(protocolInfo).find(dlnaThumbnail));
}

void Resource::addMetaData(const std::string& key, const std::string& value)
{
    m_MetaData[key] = value;
}

void Resource::setUrl(const std::string& url)
{
    m_Url = url;
}

void Resource::setProtocolInfo(const upnp::ProtocolInfo& info)
{
    m_ProtocolInfo = info;
}


Item::Item(const std::string& id, const std::string& title)
: m_ObjectId(id)
, m_ChildCount(0)
, m_Restricted(true)
{
    setTitle(title);
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

std::string Item::getTitle() const
{
    auto iter = m_MetaData.find(Property::Title);
    return (iter == m_MetaData.end()) ? "" : iter->second;
}

bool Item::restricted() const
{
    return m_Restricted;
}

std::string Item::getAlbumArtUri(dlna::ProfileId profile) const
{
    auto iter = m_AlbumArtUris.find(profile);
    return (iter == m_AlbumArtUris.end()) ? "" : iter->second;
}

const std::vector<Resource>& Item::getResources() const
{
    return m_Resources;
}

const std::map<dlna::ProfileId, std::string>& Item::getAlbumArtUris() const
{
    return m_AlbumArtUris;
}

uint32_t Item::getChildCount() const
{
    return m_ChildCount;
}

Item::Class Item::getClass() const
{
    auto iter = m_MetaData.find(Property::Class);
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

std::string Item::getClassString() const
{
    auto iter = m_MetaData.find(Property::Class);
    if (iter == m_MetaData.end())
    {
        return "Unknown";
    }
    
    return iter->second;
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
    m_MetaData[Property::Title] = title;
}

void Item::setChildCount(uint32_t count)
{
    m_ChildCount = count;
}

void Item::setAlbumArt(dlna::ProfileId profile, const std::string& uri)
{
    m_AlbumArtUris[profile] = uri;
}

void Item::addMetaData(Property prop, const std::string& value)
{
    if (!value.empty())
    {
        m_MetaData[prop] = value;
    }
}

void Item::addResource(const Resource& resource)
{
    m_Resources.push_back(resource);
}

std::string Item::getMetaData(Property prop) const
{
    auto iter = m_MetaData.find(prop);
    if (iter != m_MetaData.end())
    {
        return iter->second;
    }
    
    return "";
}

std::map<Property, std::string> Item::getMetaData() const
{
    return m_MetaData;
}

}

