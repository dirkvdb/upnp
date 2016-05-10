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

#include "upnp/upnp.protocolinfo.h"

#include "utils/format.h"
#include "utils/stringoperations.h"
#include "upnp/upnptypes.h"

namespace upnp
{

ProtocolInfo::ProtocolInfo()
{
}

ProtocolInfo::ProtocolInfo(const std::string& protocolString)
{
    auto items = utils::stringops::tokenize(protocolString, ":");
    if (items.size() != 4)
    {
        throw Exception("Invalid protocol definition: {}", protocolString);
    }

    m_protocol          = items[0];
    m_network           = items[1];
    m_contentFormat     = items[2];
    m_additionalInfo    = items[3];
}

std::string ProtocolInfo::getProtocol() const
{
    return m_protocol;
}

std::string ProtocolInfo::getNetwork() const
{
    return m_network;
}

std::string ProtocolInfo::getContentFormat() const
{
    return m_contentFormat;
}

std::string ProtocolInfo::getAdditionalInfo() const
{
    return m_additionalInfo;
}

bool ProtocolInfo::isCompatibleWith(const ProtocolInfo& info) const
{
    return (m_protocol == info.m_protocol && m_contentFormat == info.m_contentFormat);
}

std::string ProtocolInfo::toString() const
{
    if (m_protocol.empty())
    {
        // empty string for default constructed object
        return "";
    }

    return fmt::format("{}:{}:{}:{}", m_protocol, m_network, m_contentFormat, m_additionalInfo);
}

}
