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
: m_profileId(ProfileId::Unknown)
{
    auto tokens = utils::stringops::tokenize(info, ";");
    for (auto& token : tokens)
    {
        auto pair = utils::stringops::tokenize(token, "=");
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
}