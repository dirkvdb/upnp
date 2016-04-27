//
//  upnpdlnainfo.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#include "upnp/upnp.dlna.info.h"
#include "upnp.enumutils.h"

#include <cstring>

namespace upnp
{

using namespace dlna;

constexpr std::tuple<const char*, ProfileId> s_profileIdLookup[] {
    { "JPEG_TN", ProfileId::JpegThumbnail },
    { "JPEG_SM", ProfileId::JpegSmall },
    { "JPEG_MED", ProfileId::JpegMedium },
    { "JPEG_LRG", ProfileId::JpegLarge }
};

template<>
constexpr const std::tuple<const char*, ProfileId>* lut<ProfileId>()
{
    return s_profileIdLookup;
}

static_assert(enumCorrectNess<ProfileId>(), "ProfileId enum converion not correctly ordered or missing entries");

ProfileId dlna::profileIdFromString(const char* data, size_t size)
{
    return fromString<ProfileId>(data, size);
}

ProfileId dlna::profileIdFromString(const std::string& profile)
{
    return fromString<ProfileId>(profile.c_str(), profile.size());
}

const char* dlna::toString(ProfileId profile) noexcept
{
    return upnp::toString(profile);
}

}
