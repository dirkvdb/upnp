//    Copyright (C) 2016 Dirk Vanden Boer <dirk.vdb@gmail.com>
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

#include <asio.hpp>

#include <sys/types.h>
#include <ifaddrs.h>

namespace upnp
{

struct NetworkInterface
{
    std::string name;
    bool isLoopback;
    asio::ip::address address;
};

inline std::vector<NetworkInterface> getNetworkInterfaces()
{
    struct ifaddrs* addrs;
    if (getifaddrs(&addrs))
    {
        throw std::system_error(errno, std::system_category());
    }

    std::vector<NetworkInterface> interfaces;

    for (auto iter = addrs; iter != nullptr; iter = iter->ifa_next)
    {
        if (!((iter->ifa_flags & IFF_UP) && (iter->ifa_flags & IFF_RUNNING)))
        {
            continue;
        }

        if (iter->ifa_addr == nullptr)
        {
            continue;
        }

#ifdef __APPLE__
        /*
        * On Mac OS X getifaddrs returns information related to Mac Addresses for
        * various devices, such as firewire, etc. These are not relevant here.
        */
        if (iter->ifa_addr->sa_family == AF_LINK)
        {
            continue;
        }
#endif

        NetworkInterface intf;
        intf.name = iter->ifa_name;

        if (iter->ifa_addr->sa_family == AF_INET6)
        {
            auto* sockaddr = reinterpret_cast<sockaddr_in6*>(iter->ifa_addr);
            asio::ip::address_v6::bytes_type bytes;
            memcpy(&bytes[0], sockaddr->sin6_addr.s6_addr, 16);
            intf.address = asio::ip::address_v6(bytes, sockaddr->sin6_scope_id);
        }
        else
        {
            auto sockaddr = reinterpret_cast<sockaddr_in*>(iter->ifa_addr)->sin_addr;
            intf.address = asio::ip::address_v4(asio::detail::socket_ops::network_to_host_long(sockaddr.s_addr));
        }

        intf.isLoopback = !!(iter->ifa_flags & IFF_LOOPBACK);
        interfaces.push_back(intf);
    }

    freeifaddrs(addrs);
    return interfaces;
}

inline NetworkInterface getNetworkInterfaceV4(const std::string& name)
{
    auto itfs = getNetworkInterfaces();
    auto iter = std::find_if(itfs.begin(), itfs.end(), [&] (auto& intf) {
        return intf.name == name && intf.address.is_v4();
    });

    if (iter == itfs.end())
    {
        throw std::runtime_error("Interface not found: " + name);
    }

    return *iter;
}

}
