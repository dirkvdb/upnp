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

#ifndef UPNP_DEVICE_SCANNER
#define UPNP_DEVICE_SCANNER

#include <map>
#include <string>
#include <mutex>
#include <memory>
#include <future>

#include <upnp/upnp.h>

#include "upnp/upnpdevice.h"
#include "upnp/upnpclientinterface.h"
#include "upnp/upnpxmlutils.h"
#include "utils/signal.h"

namespace upnp
{

class DeviceScanner
{
public:
    DeviceScanner(IClient& client, Device::Type type);
    ~DeviceScanner() throw();
    
    void start();
    void stop();
    void refresh();
    
    uint32_t getDeviceCount();
    std::map<std::string, std::shared_ptr<Device>> getDevices();
    
    utils::Signal<void(std::shared_ptr<Device>)> DeviceDiscoveredEvent;
    utils::Signal<void(std::shared_ptr<Device>)> DeviceDissapearedEvent;
    
    void onDeviceDiscovered(Upnp_Discovery* pDiscovery);
    void onDeviceDissapeared(const std::string& deviceId);
    
private:
    static std::string getFirstDocumentItem(xml::Document& doc, const std::string& item);
    static xml::NodeList getFirstServiceList(xml::Document& doc);
    static bool findAndParseService(xml::Document& doc, ServiceType serviceType, std::shared_ptr<Device>& device);
    
    void checkForTimeoutThread();
    
    IClient&                                        m_Client;
    Device::Type                                    m_Type;
    std::map<std::string, std::shared_ptr<Device>>  m_Devices;
    std::mutex                                      m_Mutex;
    
    std::future<void>                               m_Thread;
    std::condition_variable                         m_Condition;
    bool                                            m_Started;
    bool                                            m_Stop;
    
};

}

#endif