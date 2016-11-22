//
//  upnpdlnainfo.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 10/11/12.
//  Copyright (c) 2012 None. All rights reserved.
//

#include "upnp/upnp.dlna.info.h"
#include "upnp.enumutils.h"
#include "upnp/stringview.h"
#include "utils/stringoperations.h"

#include <cstring>

namespace upnp
{

using namespace dlna;

static constexpr EnumMap<ProfileId> s_profileIdNames {{
    std::make_tuple("JPEG_TN",  ProfileId::JpegThumbnail),
    std::make_tuple("JPEG_SM",  ProfileId::JpegSmall),
    std::make_tuple("JPEG_MED", ProfileId::JpegMedium),
    std::make_tuple("JPEG_LRG", ProfileId::JpegLarge),
    std::make_tuple("PNG_TN",   ProfileId::PngThumbnail),
}};

ADD_ENUM_MAP(ProfileId, s_profileIdNames)

ProfileId dlna::profileIdFromString(const char* data, size_t size)
{
    return enum_cast<ProfileId>(std::string_view(data, size));
}

ProfileId dlna::profileIdFromString(const std::string& profile)
{
    return enum_cast<ProfileId>(profile);
}

const char* dlna::toString(ProfileId profile) noexcept
{
    return enum_string(profile);
}

Info::Info(const std::string& info)
: m_profileId(ProfileId::Unknown)
{
    auto tokens = utils::stringops::tokenize(info, ';');
    for (auto& token : tokens)
    {
        auto pair = utils::stringops::tokenize(token, '=');
        if (pair.size() == 2)
        {
            if (pair[0] == "DLNA.ORG_PN")
            {
                m_profileId = profileIdFromString(pair[1]);
            }
        }
    }
}

ProfileId Info::getProfileId() const
{
    return m_profileId;
}

}
