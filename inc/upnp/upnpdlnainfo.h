//
//  upnpdlnainfo.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#ifndef UPNP_DLNA_INFO
#define UPNP_DLNA_INFO

#include <string>

namespace upnp
{
namespace dlna
{

enum class ContentType
{
    JpegThumbnail,
    JpegSmall,
    JpegMedium,
    JpegLarge,
    Unknown
};

class Info
{
public:
    Info(const std::string& info);
    
    ContentType getContentType() const;
    
private:
    static ContentType ContentTypeFromString(const std::string& contentType);
    
    ContentType     m_contentType;
};

}
}

#endif
