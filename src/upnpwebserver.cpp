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
#include "upnp/upnputils.h"

#include <upnp/upnp.h>
#include <chrono>
#include <cstring>
#include <sstream>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/fileoperations.h"

using namespace utils;
using namespace std::chrono;

namespace upnp
{

std::mutex WebServer::m_Mutex;
std::map<std::string, std::vector<HostedFile>> WebServer::m_ServedFiles;
std::vector<WebServer::FileHandle> WebServer::m_OpenHandles;

WebServer::WebServer(const std::string& webRoot)
: m_WebRoot(webRoot)
{
    handleUPnPResult(UpnpSetWebServerRootDir(m_WebRoot.c_str()));
    
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
}

std::string WebServer::getWebRootUrl()
{
    return stringops::format("http://%s:%d/", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void WebServer::addFile(const std::string& virtualDirName, const HostedFile& file)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    //auto path = stringops::format("/%s/%s/%s", m_WebRoot, virtualDirName, file.filename);
    m_ServedFiles["/" + virtualDirName].push_back(file);
}

void WebServer::clearFiles()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ServedFiles.clear();
}

void WebServer::addVirtualDirectory(const std::string& virtualDirName)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (UPNP_E_SUCCESS != UpnpAddVirtualDir(virtualDirName.c_str()))
    {
        throw std::logic_error("Failed to add virtual directory to webserver");
    }
}

void WebServer::removeVirtualDirectory(const std::string& virtualDirName)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    UpnpRemoveVirtualDir(virtualDirName.c_str());
    m_ServedFiles.erase(virtualDirName);
}

HostedFile& WebServer::getFileFromRequest(const std::string& uri)
{
    std::string dir;
    fileops::getPathFromFilepath(uri, dir);
    auto filename = fileops::getFileName(uri);
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto iter = m_ServedFiles.find(dir);
    if (iter == m_ServedFiles.end())
    {
        throw std::logic_error("Virtual directory does not exist: " + dir);
    }
    
    auto fileIter = std::find_if(iter->second.begin(), iter->second.end(), [&] (const HostedFile& file) {
        return file.filename == filename;
    });
    
    if (fileIter == iter->second.end())
    {
        throw std::logic_error("File is not hosted: " + filename);
    }
    
    return *fileIter;
}
    
UpnpWebFileHandle WebServer::openCallback(const char* pFilename, UpnpOpenFileMode mode)
{
    if (mode == UPNP_WRITE)
    {
        // writing is not supported
        return nullptr;
    }

    FileHandle handle;
    handle.filename = pFilename;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_OpenHandles.push_back(handle);
    
    return &(m_OpenHandles.back());
}

int WebServer::getInfoCallback(const char* pFilename, File_Info* pInfo)
{
    if (pFilename == nullptr || pInfo == nullptr)
    {
        return UPNP_E_INVALID_ARGUMENT;
    }

    try
    {
        auto& file = getFileFromRequest(pFilename);
        
        pInfo->file_length = file.fileContents.size();
        pInfo->last_modified = system_clock::to_time_t(system_clock::now());
        pInfo->is_directory = 0;
        pInfo->is_readable = 1;
        pInfo->content_type = ixmlCloneDOMString(file.contentType.c_str());
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
    
    return UPNP_E_SUCCESS;
}

int WebServer::readCallback(UpnpWebFileHandle fileHandle, char* buf, size_t buflen)
{
    if (buf == nullptr)
    {
        return UPNP_E_INVALID_ARGUMENT;
    }

    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    if (pHandle->filename.empty())
    {
        return UPNP_E_INVALID_ARGUMENT;
    }
    
    try
    {
        log::debug("Read callback: buflen %d", buflen);
        auto& file = getFileFromRequest(pHandle->filename);
        if (pHandle->offset == file.fileContents.size())
        {
            return 0;
        }
        
        assert(pHandle->offset < file.fileContents.size());
        if (pHandle->offset + buflen > file.fileContents.size())
        {
            buflen = file.fileContents.size() - pHandle->offset;
        }
        
        memcpy(buf, &file.fileContents[pHandle->offset], buflen);
        pHandle->offset += buflen;
        
        return buflen;
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
}
    
int WebServer::writeCallback(UpnpWebFileHandle /*fileHandle*/, char* /*buf*/, size_t /*buflen*/)
{
    // we do not support writing of files
    return UPNP_E_INVALID_ARGUMENT;
}
    
int WebServer::seekCallback(UpnpWebFileHandle fileHandle, off_t offset, int origin)
{
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    
    try
    {
        auto& file = getFileFromRequest(pHandle->filename);

        int64_t newPosition;
        switch (origin)
        {
            case SEEK_CUR:
                newPosition = pHandle->offset + offset;
                break;
            case SEEK_END:
                newPosition = file.fileContents.size() - offset;
                break;
            case SEEK_SET:
                newPosition = offset;
                break;
            default:
                return UPNP_E_INVALID_ARGUMENT;
        }
        
        if (newPosition < 0 || newPosition >= file.fileContents.size())
        {
            return UPNP_E_INVALID_ARGUMENT;
        }
        
        pHandle->offset = newPosition;
        
        return UPNP_E_SUCCESS;
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
}
    
int WebServer::closeCallback(UpnpWebFileHandle fileHandle)
{
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    
    try
    {
        getFileFromRequest(pHandle->filename);
        return UPNP_E_SUCCESS;
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
}

}
