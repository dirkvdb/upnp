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

#ifndef UPNP_ITEM_H
#define UPNP_ITEM_H

#include <string>
#include <vector>
#include <map>

namespace upnp
{

typedef std::map<std::string, std::string> MetaMap;

class Resource
{
public:
    enum MetaData
    {
        ProtocolInfo,
        Size,
        Resolution
    };

    Resource();
    Resource(const Resource& other);
    Resource(Resource&& other);

    Resource& operator=(const Resource& other);
    Resource& operator=(Resource&& other);

    const std::string& getMetaData(MetaData meta) const;
    const std::string& getUrl() const;
    bool isThumbnail() const;
    
    void setUrl(const std::string& url);
    void addMetaData(const std::string& key, const std::string& value);

private:
    static const std::string& getMetaString(MetaData meta);

    MetaMap         m_MetaData;
    std::string     m_Url;
};

class Item
{
public:
    enum Class
    {
        Container,
        VideoContainer,
        AudioContainer,
        ImageContainer,
        Video,
        Audio,
        Image,
        Generic,
        Unknown
    };

    explicit Item(const std::string& id = "0", const std::string& title = "");
    Item(const Item& other);
    Item(Item&& other);
    virtual ~Item();
    
    Item& operator= (const Item& other);
    Item& operator= (Item&& other);
        
    const std::string& getObjectId() const;
    const std::string& getParentId() const;
    const std::string& getTitle() const;
    const std::vector<Resource>& getResources() const;
    
    uint32_t getChildCount() const;
    Class getClass() const;
    
    void setObjectId(const std::string& id);
    void setParentId(const std::string& id);
    void setTitle(const std::string& title);
    void setChildCount(uint32_t count);
        
    void addMetaData(const std::string& key, const std::string& value);
    void addResource(const Resource& resource);
    
    std::string getMetaData(const std::string& key) const;
    
    
private:
    std::string             m_ObjectId;
    std::string             m_ParentId;
    MetaMap                 m_MetaData;
    std::vector<Resource>   m_Resources;
    uint32_t                m_ChildCount;
};

}

#endif

