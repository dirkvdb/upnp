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

enum class ProfileId
{
    JpegThumbnail,  // Max 160x160
    JpegSmall,      // Max 640x640
    JpegMedium,     // Max 1024x768
    JpegLarge,
    Unknown
};

inline ProfileId profileIdFromString(const std::string& profile)
{
    if (profile == "JPEG_TN")   return ProfileId::JpegThumbnail;  
    if (profile == "JPEG_SM")   return ProfileId::JpegSmall;      
    if (profile == "JPEG_MED")  return ProfileId::JpegMedium;     
    if (profile == "JPEG_LRG")  return ProfileId::JpegLarge;
    
    return ProfileId::Unknown;
}

inline std::string toString(ProfileId profile)
{
    switch (profile)
    {
    case ProfileId::JpegThumbnail:  return "JPEG_TN";
    case ProfileId::JpegSmall:      return "JPEG_SM";
    case ProfileId::JpegMedium:     return "JPEG_MED";
    case ProfileId::JpegLarge:      return "JPEG_LRG";
    default:
        return "Unknown";
    }
}

class Info
{
public:
    Info(const std::string& info);
    
    ProfileId getProfileId() const;
    
private:
    ProfileId     m_profileId;
};

}
}

#endif
