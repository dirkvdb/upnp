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

#include "upnp/upnpdevicescanner.h"
#include "upnp/upnpclient.h"

#include "utils/log.h"

#include <upnp/upnptools.h>

using namespace utils;
using namespace std::placeholders;

static const char* MediaServerDeviceType            = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* MediaRendererDeviceType          = "urn:schemas-upnp-org:device:MediaRenderer:1";
static const char* ContentDirectoryServiceType      = "urn:schemas-upnp-org:service:ContentDirectory:1";
static const char* RenderingControlServiceType      = "urn:schemas-upnp-org:service:RenderingControl:1";
static const char* ConnectionManagerServiceType     = "urn:schemas-upnp-org:service:ConnectionManager:1";
static const char* AVTransportServiceType           = "urn:schemas-upnp-org:service:AVTransport:1";

namespace upnp
{

DeviceScanner::DeviceScanner(Client& client, Device::Type type)
: m_Client(client)
, m_Type(type)
{
}
    
void DeviceScanner::onDeviceDissapeared(const std::string& deviceId)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_Devices.find(deviceId);
    if (iter != m_Devices.end())
    {
        DeviceDissapearedEvent(iter->second);
        m_Devices.erase(iter);
    }
}

void DeviceScanner::start()
{
    m_Client.UPnPDeviceDiscoveredEvent.connect(std::bind(&DeviceScanner::onDeviceDiscovered, this, _1), this);
    m_Client.UPnPDeviceDissapearedEvent.connect(std::bind(&DeviceScanner::onDeviceDissapeared, this, _1), this);
}
    
void DeviceScanner::stop()
{
    m_Client.UPnPDeviceDiscoveredEvent.disconnect(this);
    m_Client.UPnPDeviceDissapearedEvent.disconnect(this);
}

const char* deviceTypeToString(Device::Type type)
{
    switch (type)
    {
    case Device::MediaServer:
        return MediaServerDeviceType;
    case Device::MediaRenderer:
        return MediaRendererDeviceType;
    default:
        throw std::logic_error("Invalid device type encountered");
    }
}

void DeviceScanner::refresh()
{
    log::debug("Send UPnP discovery");
    
    int rc = UpnpSearchAsync(m_Client, 5, deviceTypeToString(m_Type), &m_Client);
    if (UPNP_E_SUCCESS != rc)
    {
        log::error("Error sending search request:", rc);
    }
}

uint32_t DeviceScanner::getDeviceCount()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Devices.size();
}

std::map<std::string, std::shared_ptr<Device>> DeviceScanner::getDevices()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Devices;
}

std::string DeviceScanner::getFirstDocumentItem(IXML_Document* pDoc, const std::string& item)
{
    std::string result;
    
    IXML_NodeList* pNodeList = ixmlDocument_getElementsByTagName(pDoc, item.c_str());
    if (pNodeList)
    {
        IXML_Node* pTmpNode = ixmlNodeList_item(pNodeList, 0);
        if (pTmpNode)
        {
            IXML_Node* pTextNode = ixmlNode_getFirstChild(pTmpNode);
            const char* pValue = ixmlNode_getNodeValue(pTextNode);
            if (pValue)
            {
                result = pValue;
            }
        }
        
        ixmlNodeList_free(pNodeList);
    }
    
    return result;
}

std::string DeviceScanner::getFirstElementItem(IXML_Element* pElement, const std::string& item)
{
    std::string result;
    
    IXML_NodeList* pNodeList = ixmlElement_getElementsByTagName(pElement, item.c_str());
    if (pNodeList == nullptr)
    {
        log::error("Error finding", item, "in XML Node");
        return result;
    }
    
    IXML_Node* pTmpNode = ixmlNodeList_item(pNodeList, 0);
    if (pTmpNode == nullptr)
    {
        log::error("Error finding", item, "value in XML Node");
        ixmlNodeList_free(pNodeList);
        return result;
    }
    
    IXML_Node* pTextNode = ixmlNode_getFirstChild(pTmpNode);
    const char* pValue = ixmlNode_getNodeValue(pTextNode);
    if (pValue)
    {
        result = pValue;
    }
    
    ixmlNodeList_free(pNodeList);
    return result;
}

Service::Type stringToServiceType(const std::string& type)
{
    if (type == ContentDirectoryServiceType)    return Service::ContentDirectory;
    if (type == AVTransportServiceType)         return Service::AVTransport;
    if (type == ConnectionManagerServiceType)   return Service::ConnectionManager;
    if (type == RenderingControlServiceType)    return Service::RenderingControl;
    
    return Service::Unknown;
}

Device::Type stringToDeviceType(const std::string& type)
{
    if (type == MediaServerDeviceType)    return Device::MediaServer;
    if (type == MediaRendererDeviceType)  return Device::MediaRenderer;
    
    return Device::Unknown;
}

IXML_NodeList* DeviceScanner::getFirstServiceList(IXML_Document* pDoc)
{
    IXML_NodeList* pServiceList = nullptr;
    
    IXML_NodeList* pServlistNodelist = ixmlDocument_getElementsByTagName(pDoc, "serviceList");
    if (pServlistNodelist && ixmlNodeList_length(pServlistNodelist))
    {
        IXML_Node* pServlistNode = ixmlNodeList_item(pServlistNodelist, 0);
        pServiceList = ixmlElement_getElementsByTagName(reinterpret_cast<IXML_Element*>(pServlistNode), "service");
    }
    
    if (pServlistNodelist)
    {
        ixmlNodeList_free(pServlistNodelist);
    }
    
    return pServiceList;
}

bool DeviceScanner::findAndParseService(IXML_Document* pDoc, const Service::Type serviceType, std::shared_ptr<Device>& device)
{
    bool found = false;
    
    IXML_NodeList* pServiceList = getFirstServiceList(pDoc);
    if (!pServiceList)
    {
        return found;
    }
    
    std::string base = device->m_BaseURL.empty() ? device->m_Location : device->m_BaseURL;
    
    int numServices = ixmlNodeList_length(pServiceList);
    for (int i = 0; i < numServices; ++i)
    {
        IXML_Element* pService = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pServiceList, i));
        
        Service service;
        service.m_Type = stringToServiceType(getFirstElementItem(pService, "serviceType"));
        
        if (service.m_Type == serviceType)
        {
            service.m_Id                    = getFirstElementItem(pService, "serviceId");
            
            std::string relControlURL       = getFirstElementItem(pService, "controlURL");
            std::string relEventURL         = getFirstElementItem(pService, "eventSubURL");
            
            char url[200];
            int ret = UpnpResolveURL(base.c_str(), relControlURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating controlURL from", base, "and", relControlURL);
            }
            else
            {
                service.m_ControlURL = url;
            }
            
            ret = UpnpResolveURL(base.c_str(), relEventURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                log::error("Error generating eventURL from", base, "and", relEventURL);
            }
            else
            {
                service.m_EventSubscriptionURL = url;
            }
            
            device->m_Services[serviceType] = service;
            
            found = true;
            break;
        }
    }
    
    if (pServiceList)
    {
        ixmlNodeList_free(pServiceList);
    }
    
    return found;
}


void DeviceScanner::onDeviceDiscovered(const Client::Discovery& discovery)
{
    if (m_Type != stringToDeviceType(discovery.deviceType))
    {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        auto iter = m_Devices.find(discovery.udn);
        if (iter != m_Devices.end())
        {
            // device already known
            return;
        }
    }
    
    log::debug("New device:", discovery.udn);
    
    IXML_Document* pDoc = nullptr;
    
    log::debug("Download device description from:", discovery.location);
    int ret = UpnpDownloadXmlDoc(discovery.location.c_str(), &pDoc);
    if (ret != UPNP_E_SUCCESS)
    {
        log::error("Error obtaining device description from", discovery.location, " error =", ret);
        return;
    }

    auto device = std::make_shared<Device>();
    
    device->m_Location   = discovery.location;
    device->m_UDN        = getFirstDocumentItem(pDoc, "UDN");
    device->m_Type       = stringToDeviceType(getFirstDocumentItem(pDoc, "deviceType"));
    
    if (device->m_UDN.empty() || device->m_Type != m_Type)
    {
        if (pDoc)
        {
            ixmlDocument_free(pDoc);
        }
        
        return;
    }
    
    
    {
        device->m_FriendlyName   = getFirstDocumentItem(pDoc, "friendlyName");
        device->m_BaseURL        = getFirstDocumentItem(pDoc, "URLBase");
        device->m_RelURL         = getFirstDocumentItem(pDoc, "presentationURL");
        
        char presURL[200];
        int ret = UpnpResolveURL((device->m_BaseURL.empty() ? device->m_BaseURL.c_str() : discovery.location.c_str()), device->m_RelURL.empty() ? nullptr : device->m_RelURL.c_str(), presURL);
        if (UPNP_E_SUCCESS == ret)
        {
            device->m_PresURL = presURL;
        }
        
        if (device->m_Type == Device::MediaServer)
        {
            if (findAndParseService(pDoc, Service::ContentDirectory, device))
            {
                // try to obtain the optional services
                findAndParseService(pDoc, Service::AVTransport, device);
                findAndParseService(pDoc, Service::ConnectionManager, device);
                
                log::info("Media server added to the list:", device->m_FriendlyName, "(", device->m_UDN, ")");

                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_Devices[device->m_UDN] = device;
                }

                DeviceDiscoveredEvent(device);
            }
        }
        else if (device->m_Type == Device::MediaRenderer)
        {
            if (   findAndParseService(pDoc, Service::RenderingControl, device)
                && findAndParseService(pDoc, Service::ConnectionManager, device)
                )
            {
                // try to obtain the optional services
                findAndParseService(pDoc, Service::AVTransport, device);
                
                log::info("Media renderer added to the list:", device->m_FriendlyName, "(", device->m_UDN, ")");
                
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_Devices[device->m_UDN] = device;
                }
                
                DeviceDiscoveredEvent(device);
            }
        }
        
        if (pDoc)
        {
            ixmlDocument_free(pDoc);
        }
    }
}

}
