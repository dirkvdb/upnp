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
: m_size(0)
, m_duration(0)
, m_nrAudioChannels(0)
, m_bitRate(0)
, m_sampleRate(0)
, m_bitsPerSample(0)
{
}

const std::string& Resource::getMetaData(const std::string& metaKey) const
{
    auto iter = m_metaData.find(metaKey);
    if (iter != m_metaData.end())
    {
        return iter->second;
    }

    return emptyString;
}

const std::string& Resource::getUrl() const
{
    return m_url;
}

const ProtocolInfo& Resource::getProtocolInfo() const
{
    return m_protocolInfo;
}

uint64_t Resource::getSize() const
{
    return m_size;
}

uint32_t Resource::getDuration() const
{
    return m_duration;
}

uint32_t Resource::getNrAudioChannels() const
{
    return m_nrAudioChannels;
}

uint32_t Resource::getBitRate() const
{
    return m_bitRate;
}

uint32_t Resource::getSampleRate() const
{
    return m_sampleRate;
}

uint32_t Resource::getBitsPerSample() const
{
    return m_bitsPerSample;
}

bool Resource::isThumbnail() const
{
    return (0 != getMetaData(protocolInfo).find(dlnaThumbnail));
}

void Resource::addMetaData(const std::string& key, const std::string& value)
{
    m_metaData[key] = value;
}

void Resource::setUrl(const std::string& url)
{
    m_url = url;
}

void Resource::setProtocolInfo(const upnp::ProtocolInfo& info)
{
    m_protocolInfo = info;
}

void Resource::setSize(uint64_t size)
{
    m_size = size;
}

void Resource::setDuration(uint32_t durationInSeconds)
{
    m_duration = durationInSeconds;
}

void Resource::setNrAudioChannels(uint32_t channels)
{
    m_nrAudioChannels = channels;
}

void Resource::setBitRate(uint32_t bitRate)
{
    m_bitRate = bitRate;
}

void Resource::setSampleRate(uint32_t sampleRate)
{
    m_sampleRate = sampleRate;
}

void Resource::setBitsPerSample(uint32_t bitsPerSample)
{
    m_bitsPerSample = bitsPerSample;
}

Item::Item(const std::string& id, const std::string& title)
: m_objectId(id)
, m_restricted(true)
, m_childCount(0)
{
    setTitle(title);
}

Item::~Item()
{
}

Item& Item::operator= (const Item& other)
{
    m_objectId      = other.m_objectId;
    m_parentId      = other.m_parentId;
    m_metaData      = other.m_metaData;
    m_resources     = other.m_resources;
    m_restricted    = other.m_restricted;
    m_albumArtUris  = other.m_albumArtUris;
    m_childCount    = other.m_childCount;

    return *this;
}

Item& Item::operator= (Item&& other)
{
    m_objectId      = std::move(other.m_objectId);
    m_parentId      = std::move(other.m_parentId);
    m_metaData      = std::move(other.m_metaData);
    m_resources     = std::move(other.m_resources);
    m_restricted    = std::move(other.m_restricted);
    m_albumArtUris  = std::move(other.m_albumArtUris);
    m_childCount    = other.m_childCount;

    return *this;
}

const std::string& Item::getObjectId() const
{
    return m_objectId;
}

const std::string& Item::getParentId() const
{
    return m_parentId;
}

const std::string& Item::getRefId() const
{
    return m_refId;
}

const std::string& Item::getTitle() const
{
    return getMetaData(upnp::Property::Title);
}

bool Item::restricted() const
{
    return m_restricted;
}

bool Item::isContainer() const
{
    auto iter = m_metaData.find(Property::Class);
    if (iter == m_metaData.end())
    {
        return false;
    }

    return iter->second.find("object.container") == 0;
}

std::string Item::getAlbumArtUri(dlna::ProfileId profile) const
{
    auto iter = m_albumArtUris.find(profile);
    return (iter == m_albumArtUris.end()) ? "" : iter->second;
}

const std::vector<Resource>& Item::getResources() const
{
    return m_resources;
}

const std::map<dlna::ProfileId, std::string>& Item::getAlbumArtUris() const
{
    return m_albumArtUris;
}

uint32_t Item::getChildCount() const
{
    return m_childCount;
}

Class Item::getClass() const
{
    auto iter = m_metaData.find(Property::Class);
    if (iter == m_metaData.end())
    {
        return Class::Unknown;
    }

    const std::string& upnpClass = iter->second;
    if (0 == upnpClass.find("object.item.audioItem"))
    {
        return Class::Audio;
    }
    else if (0 == upnpClass.find("object.item.imageItem"))
    {
        return Class::Image;
    }
    else if (0 == upnpClass.find("object.item.videoItem"))
    {
        return Class::Video;
    }
    else if (upnpClass == "object.item")
    {
        return Class::Generic;
    }
    else if (upnpClass == "object.container.videoContainer")
    {
        return Class::VideoContainer;
    }
    else if (upnpClass == "object.container.storageFolder")
    {
        return Class::StorageFolder;
    }
    else if (upnpClass == "object.container.album.musicAlbum")
    {
        return Class::AudioContainer;
    }
    else if (upnpClass == "object.container.album.photoAlbum")
    {
        return Class::ImageContainer;
    }
    else if (0 == upnpClass.find("object.container"))
    {
        return Class::Container;
    }

    return Class::Unknown;
}

void Item::setClass(Class c)
{
    m_metaData.emplace(Property::Class, toString(c));
}

void Item::setClass(const std::string& className)
{
    m_metaData.emplace(Property::Class, className);
}

std::string Item::getClassString() const
{
    auto iter = m_metaData.find(Property::Class);
    if (iter == m_metaData.end())
    {
        return "Unknown";
    }

    return iter->second;
}

void Item::setObjectId(const std::string& id)
{
    m_objectId = id;
}

void Item::setParentId(const std::string& id)
{
    m_parentId = id;
}

void Item::setRefId(const std::string& id)
{
    m_refId = id;
}

void Item::setTitle(const std::string& title)
{
    m_metaData[Property::Title] = title;
}

void Item::setChildCount(uint32_t count)
{
    m_childCount = count;
}

void Item::setAlbumArt(dlna::ProfileId profile, const std::string& uri)
{
    m_albumArtUris[profile] = uri;
}

void Item::addMetaData(Property prop, const std::string& value)
{
    if (!value.empty())
    {
        m_metaData[prop] = value;
    }
}

void Item::addResource(const Resource& resource)
{
    m_resources.push_back(resource);
}

const std::string& Item::getMetaData(Property prop) const
{
    auto iter = m_metaData.find(prop);
    if (iter != m_metaData.end())
    {
        return iter->second;
    }

    return emptyString;
}

std::map<Property, std::string> Item::getMetaData() const
{
    return m_metaData;
}

}

