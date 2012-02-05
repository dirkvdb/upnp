//
//  upnpdevicescanner.h
//  UPnPFramework
//
//  Created by Dirk Vanden Boer on 03/06/11.
//  Copyright 2011 None. All rights reserved.
//

#ifndef UPNP_DEVICE_SCANNER
#define UPNP_DEVICE_SCANNER

#include <map>
#include <string>
#include <mutex>
#include <Utils/subscriber.h>

#include "upnpdevice.h"

namespace UPnP
{

class ControlPoint;

class DeviceScanner : public IDeviceSubscriber
{
public:
    enum DeviceType
    {
        Servers
    };
    
    DeviceScanner(ControlPoint& controlPoint, DeviceType type);
    
    void setListener(IDeviceSubscriber& listener);
    
    void start();
    void stop();
    void refresh();
    
    uint32_t getDeviceCount();
    std::map<std::string, Device> getDevices();
    
    void onUPnPDeviceEvent(const Device& device, DeviceEvent event);
    
private:
    ControlPoint&                   m_ControlPoint;
    DeviceType                      m_Type;
    std::map<std::string, Device>   m_Devices;
    std::mutex                      m_Mutex;
    
    IDeviceSubscriber*              m_pListener;
};

}

#endif