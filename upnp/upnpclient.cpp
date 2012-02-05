//
//  upnpclient.cpp
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 06/06/11.
//  Copyright 2011 None. All rights reserved.
//

#include "upnpclient.h"

#include "Utils/stringoperations.h"

namespace UPnP
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

void Client::getMetaData(UPnP::Item& item)
{
    m_Browser.getMetaData(item, "");
}

}