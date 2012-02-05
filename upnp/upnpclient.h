//
//  upnpclient.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 06/06/11.
//  Copyright 2011 None. All rights reserved.
//

#ifndef UPNP_CLIENT_H
#define UPNP_CLIENT_H

#include "upnpbrowser.h"

namespace UPnP
{

class ControlPoint;

class Client
{
public:
    Client(const ControlPoint& cp, const Device& device);
    
    void getItemsInContainer(Item& container, utils::ISubscriber<Item>& subscriber);
    void getContainersInContainer(Item& container, utils::ISubscriber<Item>& subscriber);
    void getAllInContainer(Item& container, utils::ISubscriber<Item>& subscriber);
    
    void getMetaData(Item& item);
    
private:
    Browser     m_Browser;
}; 

}

#endif