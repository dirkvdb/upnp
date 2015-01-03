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
#include <set>
#include <string>
#include <mutex>
#include <memory>
#include <future>
#include <thread>

#include <upnp/upnp.h>

#include "upnp/upnpdevice.h"
#include "upnp/upnpclientinterface.h"
#include "upnp/upnpxmlutils.h"
#include "utils/signal.h"
#include "utils/workerthread.h"
#include "utils/threadpool.h"

namespace upnp
{

class DeviceScanner
{
public:
    DeviceScanner(IClient& client, DeviceType type);
    DeviceScanner(IClient& client, std::set<DeviceType> types);
    ~DeviceScanner() throw();
    
    void start();
    void stop();
    void refresh();
    
    uint32_t getDeviceCount() const;
    std::shared_ptr<Device> getDevice(const std::string& udn) const;
    std::map<std::string, std::shared_ptr<Device>> getDevices() const;
    
    utils::Signal<std::shared_ptr<Device>> DeviceDiscoveredEvent;
    utils::Signal<std::shared_ptr<Device>> DeviceDissapearedEvent;
    
private:
    void onDeviceDiscovered(const DeviceDiscoverInfo& info);
    void onDeviceDissapeared(const std::string& deviceId);
    void updateDevice(const DeviceDiscoverInfo& info, const std::shared_ptr<Device>& device);
    void obtainDeviceDetails(const DeviceDiscoverInfo& info, const std::shared_ptr<Device>& device);
    static xml::NodeList getFirstServiceList(xml::Document& doc);
    static bool findAndParseService(xml::Document& doc, ServiceType serviceType, const std::shared_ptr<Device>& device);
    
    void checkForTimeoutThread();
    
    IClient&                                        m_Client;
    const std::set<DeviceType>                      m_Types;
    std::map<std::string, std::shared_ptr<Device>>  m_Devices;
    mutable std::mutex                              m_Mutex;
    mutable std::mutex                              m_DataMutex;
    
    std::future<void>                               m_Thread;
    utils::ThreadPool                               m_DownloadPool;
    std::condition_variable                         m_Condition;
    bool                                            m_Started;
    bool                                            m_Stop;
    
};

}

#endif