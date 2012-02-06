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

#ifndef UPNP_CONTROL_POINT_H
#define UPNP_CONTROL_POINT_H

#include <string>
#include <list>
#include <mutex>
#include <upnp/upnp.h>

#include "upnpdevice.h"
#include "utils/subscriber.h"


namespace UPnP
{
    
class ControlPoint
{
public:
    ControlPoint();
    ~ControlPoint();
    
    void initialize();
    void destroy();

    operator UpnpClient_Handle() const { return m_CtrlPnt; }
    void getServersASync(IDeviceSubscriber& subscriber);
    void stopReceivingServers(IDeviceSubscriber& subscriber);
    void manualDiscovery();

    void reset();
    
private:
    
    static std::string getFirstDocumentItem(IXML_Document* pDoc, const std::string& item);
    static std::string getFirstElementItem(IXML_Element* pElement, const std::string& item);
    static IXML_NodeList* getFirstServiceList(IXML_Document* pDoc);
    static bool findAndParseService(IXML_Document* pDoc, const std::string serviceType, Device& device);

    void onDeviceDiscovered(IXML_Document* pDoc, char* pLocation, int expires);
    void onDeviceDissapeared(const char* deviceId);

    static int cpCb(Upnp_EventType EventType, void* pEvent, void* pcookie);

    UpnpClient_Handle                   		m_CtrlPnt;
    std::mutex                                  m_Mutex;
    std::list<IDeviceSubscriber*>               m_DeviceSubscribers;
};

}

#endif

