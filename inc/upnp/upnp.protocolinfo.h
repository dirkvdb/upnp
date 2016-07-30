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

#pragma once

#include <string>

namespace upnp
{

class ProtocolInfo
{
public:
    ProtocolInfo() = default;
    ProtocolInfo(const std::string& protocolString);

    std::string getProtocol() const noexcept;
    std::string getNetwork() const noexcept;
    std::string getContentFormat() const noexcept;
    std::string getAdditionalInfo() const noexcept;

    bool isCompatibleWith(const ProtocolInfo& info) const noexcept;
    bool operator==(const ProtocolInfo& info) const noexcept;

    std::string toString() const noexcept;

private:
    std::string m_protocol;
    std::string m_network;
    std::string m_contentFormat;
    std::string m_additionalInfo;
};

}

std::ostream& operator<< (std::ostream& os, const upnp::ProtocolInfo& info);
