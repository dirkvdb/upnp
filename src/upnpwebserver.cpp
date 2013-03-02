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

#include "upnp/upnpwebserver.h"

#include <upnp/upnp.h>
#include <chrono>
#include <sstream>

#include "utils/log.h"

using namespace std::chrono;

namespace upnp
{

std::map<std::string, std::string> WebServer::m_ServedFiles;
std::vector<WebServer::FileHandle> WebServer::m_OpenHandles;

WebServer::WebServer(const std::string& webRoot)
: m_WebRoot(webRoot)
{
}

std::string WebServer::getWebRootUrl()
{
    std::stringstream ss;
    ss << "http://" << UpnpGetServerIpAddress() << ":" << UpnpGetServerPort() << "/medilna/";

    return ss.str();
}

void WebServer::addFile(const std::string& filename, const std::string& fileContents)
{
    std::stringstream ss;
    ss << "/medilna" << "/" << filename;
    m_ServedFiles.insert(std::make_pair(ss.str(), fileContents));
}

void WebServer::clearFiles()
{
    m_ServedFiles.clear();
}

void WebServer::start()
{
    UpnpVirtualDirCallbacks cbs;
    cbs.get_info    = &WebServer::getInfoCallback;
    cbs.open        = &WebServer::openCallback;
    cbs.read        = &WebServer::readCallback;
    cbs.write       = &WebServer::writeCallback;
    cbs.seek        = &WebServer::seekCallback;
    cbs.close       = &WebServer::closeCallback;
    
    if (UPNP_E_SUCCESS != UpnpSetVirtualDirCallbacks(&cbs))
    {
        throw std::logic_error("Failed to create webserver");
    }
    
    if (UPNP_E_SUCCESS != UpnpAddVirtualDir("medilna"))
    {
        throw std::logic_error("Failed to create webserver");
    }
}

void WebServer::stop()
{
    UpnpRemoveVirtualDir("medilna");
}

UpnpWebFileHandle WebServer::openCallback(const char* pFilename, UpnpOpenFileMode mode)
{
    utils::log::debug("% %", __FUNCTION__, pFilename);

    if (mode == UPNP_WRITE)
    {
        // writing is not supported
        return nullptr;
    }

    FileHandle handle;
    handle.filename = pFilename;
    
    m_OpenHandles.push_back(handle);
    
    return &(m_OpenHandles.back());
}

int WebServer::getInfoCallback(const char* pFilename, File_Info* pInfo)
{
    utils::log::debug("% % %", __FUNCTION__, pFilename, pInfo);
    auto iter = m_ServedFiles.find(pFilename);
    if (iter != m_ServedFiles.end())
    {
        pInfo->file_length = iter->second.size();
        pInfo->last_modified = system_clock::to_time_t(system_clock::now());
        pInfo->is_directory = 0;
        pInfo->is_readable = 1;
        pInfo->content_type = ixmlCloneDOMString("audio/m3u");
    }
    
    return UPNP_E_SUCCESS;
}

int WebServer::readCallback(UpnpWebFileHandle fileHandle, char* buf, size_t buflen)
{
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);

    auto iter = m_ServedFiles.find(pHandle->filename);
    if (iter == m_ServedFiles.end())
    {
        return UPNP_E_INVALID_ARGUMENT;
    }
    
    if (pHandle->offset == iter->second.size())
    {
        return 0;
    }
    
    if (pHandle->offset + buflen > iter->second.size())
    {
        buflen = iter->second.size() - pHandle->offset;
    }
    
    memcpy(buf, &iter->second[pHandle->offset], buflen);
    pHandle->offset += buflen;
    
    utils::log::debug("% Read bytes: % Offset: %", __FUNCTION__, buflen, pHandle->offset);

    return buflen;
}
    
int WebServer::writeCallback(UpnpWebFileHandle /*fileHandle*/, char* /*buf*/, size_t /*buflen*/)
{
    // we do not support writing of files
    return UPNP_E_INVALID_ARGUMENT;
}
    
int WebServer::seekCallback(UpnpWebFileHandle fileHandle, off_t offset, int origin)
{
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    
    auto iter = m_ServedFiles.find(pHandle->filename);
    if (iter == m_ServedFiles.end())
    {
        return UPNP_E_INVALID_ARGUMENT;
    }

    int64_t newPosition;
    switch (origin)
    {
        case SEEK_CUR:
            newPosition = pHandle->offset + offset;
            break;
        case SEEK_END:
            newPosition = iter->second.size() - offset;
            break;
        case SEEK_SET:
            newPosition = offset;
            break;
        default:
            return UPNP_E_INVALID_ARGUMENT;
    }
    
    if (newPosition < 0 || newPosition >= iter->second.size())
    {
        return UPNP_E_INVALID_ARGUMENT;
    }
    
    pHandle->offset = newPosition;
    
    utils::log::debug("% new position: %", __FUNCTION__, pHandle->offset);

    return UPNP_E_SUCCESS;
}
    
int WebServer::closeCallback(UpnpWebFileHandle fileHandle)
{
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    
    auto iter = m_ServedFiles.find(pHandle->filename);
    if (iter == m_ServedFiles.end())
    {
        return UPNP_E_INVALID_ARGUMENT;
    }

    utils::log::debug(__FUNCTION__);
    return UPNP_E_SUCCESS;
}

}
