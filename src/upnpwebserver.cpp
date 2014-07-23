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
#include <map>
#include <mutex>
#include <memory>

#include <upnp/upnp.h>

#include "utils/log.h"
#include "utils/stringoperations.h"
#include "utils/fileoperations.h"

using namespace utils;
using namespace std::chrono;

namespace upnp
{

namespace
{

struct HostedFile
{
    std::string             filename;
    std::vector<uint8_t>    fileContents;
    std::string             contentType;
};

struct FileHandle
{
    FileHandle() : offset(0) {}

    uint64_t		id;
    std::string     filename;
    size_t          offset;
};

std::mutex g_mutex;
uint64_t g_currentRequestId = 0;
std::map<std::string, std::vector<std::unique_ptr<HostedFile>>> g_servedFiles;
std::map<std::string, std::function<void(const std::string&)>> g_virtualDirs;
std::vector<std::unique_ptr<FileHandle>> g_openHandles;


HostedFile& getFileFromRequest(const std::string& uri)
{
	std::string dir = fileops::getPathFromFilepath(uri);
    auto filename = fileops::getFileName(uri);
    auto iter = g_servedFiles.find(dir);
    if (iter == g_servedFiles.end())
    {
        throw std::logic_error("Virtual directory does not exist: " + dir);
    }
    
    auto fileIter = std::find_if(iter->second.begin(), iter->second.end(), [&] (const std::unique_ptr<HostedFile>& file) {
        return file->filename == filename;
    });
    
    if (fileIter == iter->second.end())
    {
        throw std::logic_error("File is not hosted: " + filename);
    }
    
    return (*(*fileIter).get());
}

UpnpWebFileHandle openCallback(const char* pFilename, UpnpOpenFileMode mode)
{
    if (mode == UPNP_WRITE)
    {
        // writing is not supported
        return nullptr;
    }

    std::unique_ptr<FileHandle> handle(new FileHandle());
    handle->filename = pFilename;
    handle->offset = 0;
    handle->id = ++g_currentRequestId;
    
    std::lock_guard<std::mutex> lock(g_mutex);
    g_openHandles.push_back(std::move(handle));
    
    return g_openHandles.back().get();
}

int getInfoCallback(const char* pFilename, File_Info* pInfo)
{
    if (pFilename == nullptr || pInfo == nullptr)
    {
        return UPNP_E_INVALID_ARGUMENT;
    }
    
    try
    {
    	std::lock_guard<std::mutex> lock(g_mutex);
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

int readCallback(UpnpWebFileHandle fileHandle, char* buf, size_t buflen)
{
	std::lock_guard<std::mutex> lock(g_mutex);

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
        
        return static_cast<int>(buflen);
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
}
    
int writeCallback(UpnpWebFileHandle /*fileHandle*/, char* /*buf*/, size_t /*buflen*/)
{
    // we do not support writing of files
    return UPNP_E_INVALID_ARGUMENT;
}
    
int seekCallback(UpnpWebFileHandle fileHandle, off_t offset, int origin)
{
	std::lock_guard<std::mutex> lock(g_mutex);
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
    
int closeCallback(UpnpWebFileHandle fileHandle)
{
	std::lock_guard<std::mutex> lock(g_mutex);
    FileHandle* pHandle = reinterpret_cast<FileHandle*>(fileHandle);
    
    try
    {
		g_openHandles.erase(std::remove_if(g_openHandles.begin(), g_openHandles.end(), [&] (const std::unique_ptr<FileHandle>& file) {
			return file->id == pHandle->id;
		}), g_openHandles.end());

        return UPNP_E_SUCCESS;
    }
    catch (std::exception& e)
    {
        log::error(e.what());
        return UPNP_E_INVALID_ARGUMENT;
    }
}

}

WebServer::WebServer(const std::string& webRoot)
: m_webRoot(webRoot)
{
    handleUPnPResult(UpnpSetWebServerRootDir(m_webRoot.c_str()));
    
    UpnpVirtualDirCallbacks cbs;
    cbs.get_info    = &getInfoCallback;
    cbs.open        = &openCallback;
    cbs.read        = &readCallback;
    cbs.write       = &writeCallback;
    cbs.seek        = &seekCallback;
    cbs.close       = &closeCallback;
    
    if (UPNP_E_SUCCESS != UpnpSetVirtualDirCallbacks(&cbs))
    {
        throw std::logic_error("Failed to create webserver");
    }
}

WebServer::~WebServer()
{
    clearFiles();
    
    UpnpRemoveAllVirtualDirs();
    UpnpSetWebServerRootDir(nullptr);
}

std::string WebServer::getWebRootUrl()
{
    return stringops::format("http://%s:%d/", UpnpGetServerIpAddress(), UpnpGetServerPort());
}

void WebServer::addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::string& data)
{
	std::unique_ptr<HostedFile> file(new HostedFile());
	file->filename       = filename;
	file->contentType    = contentType;
	file->fileContents.resize(data.size());
    memcpy(file->fileContents.data(), data.data(), data.size());
    
    std::lock_guard<std::mutex> lock(g_mutex);
    g_servedFiles["/" + virtualDir].push_back(std::move(file));
}

void WebServer::addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::vector<uint8_t>& data)
{
	std::unique_ptr<HostedFile> file(new HostedFile());
    file->filename       = filename;
    file->contentType    = contentType;
    file->fileContents   = data;
    
    std::lock_guard<std::mutex> lock(g_mutex);
    g_servedFiles["/" + virtualDir].push_back(std::move(file));
}

void WebServer::removeFile(const std::string& virtualDir, const std::string& filename)
{
    auto& files = g_servedFiles["/" + virtualDir];
    files.erase(std::remove_if(files.begin(), files.end(), [&] (const std::unique_ptr<HostedFile>& file) {
        return file->filename == filename;
    }), files.end());
}

void WebServer::clearFiles()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_servedFiles.clear();
    g_openHandles.clear();
}

void WebServer::addVirtualDirectory(const std::string& virtualDirName)
{
	std::lock_guard<std::mutex> lock(g_mutex);

    if (UPNP_E_SUCCESS != UpnpAddVirtualDir(virtualDirName.c_str()))
    {
        throw std::logic_error("Failed to add virtual directory to webserver");
    }

    g_servedFiles.emplace("/" + virtualDirName, std::vector<std::unique_ptr<HostedFile>>());
}

void WebServer::addVirtualDirectory(const std::string& virtualDirName, std::function<void(const std::string&)> requestCb)
{
	std::lock_guard<std::mutex> lock(g_mutex);

    if (UPNP_E_SUCCESS != UpnpAddVirtualDir(virtualDirName.c_str()))
    {
        throw std::logic_error("Failed to add virtual directory to webserver");
    }

    g_virtualDirs.emplace("/" + virtualDirName, requestCb);
}

void WebServer::removeVirtualDirectory(const std::string& virtualDirName)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UpnpRemoveVirtualDir(virtualDirName.c_str());
    g_servedFiles.erase("/" + virtualDirName);
}

}
