//
//  upnpdlnainfo.cpp
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#include "upnp/upnpdlnainfo.h"

#include "utils/log.h"
#include "utils/stringoperations.h"

namespace upnp
{
namespace dlna
{

Info::Info(const std::string& info)
: m_contentType(ContentType::Unknown)
{
    auto tokens = utils::stringops::tokenize(info, ";");
    for (auto& token : tokens)
    {
        auto pair = utils::stringops::tokenize(token, "=");
        if (pair.size() == 2)
        {
            if (pair[0] == "DLNA.ORG_PN")
            {
                m_contentType = ContentTypeFromString(pair[1]);
            }
        }
    }
}
    
ContentType Info::getContentType() const
{
    return m_contentType;
}
    
ContentType Info::ContentTypeFromString(const std::string& contentType)
{
    if (contentType == "JPEG_TN")   return ContentType::JpegThumbnail;
    if (contentType == "JPEG_SM")   return ContentType::JpegSmall;
    if (contentType == "JPEG_MED")  return ContentType::JpegMedium;
    if (contentType == "JPEG_LRG")  return ContentType::JpegLarge;
    
    return ContentType::Unknown;
}

}
}