//
//  upnpdlnainfo.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#pragma once

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

ProfileId profileIdFromString(const char* data, size_t size);
ProfileId profileIdFromString(const std::string& profile);
const char* toString(ProfileId profile) noexcept;

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
