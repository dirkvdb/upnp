//
//  upnpdlnainfo.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#include "upnp/upnp.dlna.info.h"
#include "upnp.enumutils.h"
#include "gsl/string_span.h"

#include <cstring>

namespace upnp
{

using namespace dlna;

static constexpr EnumMap<ProfileId> s_profileIdNames {{
    { "JPEG_TN", ProfileId::JpegThumbnail },
    { "JPEG_SM", ProfileId::JpegSmall },
    { "JPEG_MED", ProfileId::JpegMedium },
    { "JPEG_LRG", ProfileId::JpegLarge }
}};

ADD_ENUM_MAP(ProfileId, s_profileIdNames)

ProfileId dlna::profileIdFromString(const char* data, size_t size)
{
    return enum_cast<ProfileId>(gsl::cstring_span<>(data, size));
}

ProfileId dlna::profileIdFromString(const std::string& profile)
{
    return enum_cast<ProfileId>(profile);
}

const char* dlna::toString(ProfileId profile) noexcept
{
    return string_cast(profile);
}

}
