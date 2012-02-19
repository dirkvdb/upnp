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

#include "upnpclient.h"

#include "utils/stringoperations.h"

namespace upnp
{

Client::Client(const ControlPoint& cp, const Device& device)
: m_Browser(cp)
{
    m_Browser.setDevice(device);
}

void Client::getItemsInContainer(Item& container, utils::ISubscriber<Item>& subscriber)
{
    m_Browser.getItems(subscriber, container);
}

void Client::getContainersInContainer(Item& container, utils::ISubscriber<Item>& subscriber)
{
    m_Browser.getContainers(subscriber, container);
}

void Client::getAllInContainer(Item& container, utils::ISubscriber<Item>& subscriber)
{
    m_Browser.getContainersAndItems(subscriber, container);
}

void Client::getMetaData(upnp::Item& item)
{
    m_Browser.getMetaData(item, "");
}

}