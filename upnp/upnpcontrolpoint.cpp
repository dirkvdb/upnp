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

#include "upnpcontrolpoint.h"

#include "utils/log.h"

#include <stdexcept>
#include <algorithm>
#include <upnp/upnptools.h>

static const char* MediaServerType = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* ContentDirectoryServiceType = "urn:schemas-upnp-org:service:ContentDirectory:1";

namespace UPnP
{

ControlPoint::ControlPoint()
: m_CtrlPnt(0)
{
}

ControlPoint::~ControlPoint()
{
    destroy();
}

void ControlPoint::initialize()
{
    Log::info("Initializing UPnP SDK");

    int rc = UpnpInit(nullptr, 0);
    if (UPNP_E_SUCCESS != rc && UPNP_E_INIT != rc)
    {
        UpnpFinish();
        Log::error("UpnpInit() Error:", rc);
        throw std::logic_error("Failed to initialise UPnP stack");
    }
    
    Log::info("UPnP Initialized: ipaddress=", UpnpGetServerIpAddress(), "port=", UpnpGetServerPort());
    Log::info("Registering Control Point");
    
    rc = UpnpRegisterClient(cpCb, this, &m_CtrlPnt);
    if (UPNP_E_SUCCESS == rc)
    {
        Log::info("Control Point Registered");
        UpnpSetMaxContentLength(128 * 1024);
    }
    if (UPNP_E_ALREADY_REGISTERED == rc)
    {
        Log::warn("Control Point was already registered");
    }
    else if (UPNP_E_SUCCESS != rc )
    {
        Log::error("Error registering Control Point: ", rc);
        UpnpFinish();
        throw std::logic_error("Error registering Control Point");
    }    
}
    
void ControlPoint::destroy()
{
    UpnpUnRegisterClient(m_CtrlPnt);
    UpnpFinish();
    
    Log::info("Destroyed UPnP SDK");
}

void ControlPoint::reset()
{
    UpnpUnRegisterClient(m_CtrlPnt);
    UpnpFinish();
    initialize();
}

void ControlPoint::getServersASync(IDeviceSubscriber& subscriber)
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_DeviceSubscribers.push_back(&subscriber);
    }

    manualDiscovery();
}

void ControlPoint::stopReceivingServers(IDeviceSubscriber& subscriber)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (std::list<IDeviceSubscriber*>::iterator iter = m_DeviceSubscribers.begin(); iter != m_DeviceSubscribers.end(); ++iter)
    {
        if (*iter == &subscriber)
        {
            m_DeviceSubscribers.erase(iter);
            return;
        }
    }
}

void ControlPoint::manualDiscovery()
{
    Log::debug("Send UPnP discovery");
    
    int rc = UpnpSearchAsync(m_CtrlPnt, 5, MediaServerType, this);
    if (UPNP_E_SUCCESS != rc)
    {
        Log::error("Error sending search request:", rc);
    }
}

int ControlPoint::cpCb(Upnp_EventType eventType, void* pEvent, void* pCookie)
{
    ControlPoint* pUPnP = reinterpret_cast<ControlPoint*>(pCookie);
    
    switch (eventType)
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
        struct Upnp_Discovery* pDiscEvent = (struct Upnp_Discovery*) pEvent;
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            Log::error("Error in Discovery Alive Callback:", pDiscEvent->ErrCode);
        }
        else
        {
            IXML_Document* pDoc = nullptr;
            
            int ret = UpnpDownloadXmlDoc(pDiscEvent->Location, &pDoc);
            if (ret != UPNP_E_SUCCESS)
            {
                Log::error("Error obtaining device description from", pDiscEvent->Location, " error =", ret);
            }
            else
            {
                pUPnP->onDeviceDiscovered(pDoc, pDiscEvent->Location, pDiscEvent->Expires);
            }
            
            if (pDoc)
            {
                ixmlDocument_free(pDoc);
            }
        }
        break;
    }
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
        struct Upnp_Discovery* pDiscEvent = (struct Upnp_Discovery*) pEvent;
        if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
        {
            Log::error("Error in Discovery Bye Bye Callback:", pDiscEvent->ErrCode);
        }
        else
        {
            pUPnP->onDeviceDissapeared(pDiscEvent->DeviceId);
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

std::string ControlPoint::getFirstDocumentItem(IXML_Document* pDoc, const std::string& item)
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

std::string ControlPoint::getFirstElementItem(IXML_Element* pElement, const std::string& item)
{
    std::string result;

    IXML_NodeList* pNodeList = ixmlElement_getElementsByTagName(pElement, item.c_str());
    if (pNodeList == nullptr)
    {
        Log::error("Error finding", item, "in XML Node");
        return result;
    }

    IXML_Node* pTmpNode = ixmlNodeList_item(pNodeList, 0);
    if (pTmpNode == nullptr)
    {
        Log::error("Error finding", item, "value in XML Node");
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

IXML_NodeList* ControlPoint::getFirstServiceList(IXML_Document* pDoc)
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

bool ControlPoint::findAndParseService(IXML_Document* pDoc, const std::string serviceType, Device& device)
{
    bool found = false;
    
    IXML_NodeList* pServiceList = getFirstServiceList(pDoc);
    if (!pServiceList)
    {
        return found;
    }
    
    std::string base = device.m_BaseURL.empty() ? device.m_Location : device.m_BaseURL;
    
    int numServices = ixmlNodeList_length(pServiceList);
    for (int i = 0; i < numServices; ++i)
    {
        IXML_Element* pService = reinterpret_cast<IXML_Element*>(ixmlNodeList_item(pServiceList, i));
        
        std::string tempServiceType = getFirstElementItem(pService, "serviceType");

        if (tempServiceType == serviceType)
        {
            device.m_CDServiceID        = getFirstElementItem(pService, "serviceId");
            device.m_CDEventSubURL      = getFirstElementItem(pService, "eventSubURL");
            
            std::string relControlURL   = getFirstElementItem(pService, "controlURL");
            std::string relEventURL     = getFirstElementItem(pService, "eventSubURL");
                
            char url[200];
            int ret = UpnpResolveURL(base.c_str(), relControlURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                Log::error("Error generating controlURL from", base, "and", relControlURL);
            }
            else
            {
                device.m_CDControlURL = url;
            }
            
            ret = UpnpResolveURL(base.c_str(), relEventURL.c_str(), url);
            if (ret != UPNP_E_SUCCESS)
            {
                Log::error("Error generating eventURL from", base, "and", relEventURL);
            }
            else
            {
                device.m_CDEventSubURL = url;
            }
           
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


void ControlPoint::onDeviceDiscovered(IXML_Document* pDoc, char* pLocation, int expires)
{
    Device device;

    device.m_Location   = pLocation;
    device.m_UDN        = getFirstDocumentItem(pDoc, "UDN");
    device.m_DeviceType = getFirstDocumentItem(pDoc, "deviceType");
    
    if (device.m_UDN.empty() || device.m_DeviceType != MediaServerType)
    {
        return;
    }
    
    {
        device.m_DeviceType     = getFirstDocumentItem(pDoc, "deviceType");
        device.m_FriendlyName   = getFirstDocumentItem(pDoc, "friendlyName");
        device.m_BaseURL        = getFirstDocumentItem(pDoc, "URLBase");
        device.m_RelURL         = getFirstDocumentItem(pDoc, "presentationURL");

        char presURL[200];
        int ret = UpnpResolveURL((device.m_BaseURL.empty() ? device.m_BaseURL.c_str() : pLocation ), device.m_RelURL.empty() ? nullptr : device.m_RelURL.c_str(), presURL);
        if (UPNP_E_SUCCESS == ret)
        {
            device.m_PresURL = presURL;
        }
        
        if (findAndParseService(pDoc, ContentDirectoryServiceType, device))
        {
            //Log::info("Device added to the list:", device.m_FriendlyName, "(", device.m_UDN, ")");
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (auto subscriber : m_DeviceSubscribers)
            {
                subscriber->onUPnPDeviceEvent(device, IDeviceSubscriber::deviceDiscovered);
            }
        }
    }
}

void ControlPoint::onDeviceDissapeared(const char* deviceId)
{
    Device device;
    device.m_UDN = deviceId;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto subscriber : m_DeviceSubscribers)
    {
        subscriber->onUPnPDeviceEvent(device, IDeviceSubscriber::deviceDissapeared);
    }
}

}
