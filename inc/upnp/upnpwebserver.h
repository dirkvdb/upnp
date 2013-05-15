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

#ifndef UPNP_WEBSERVER_H
#define UPNP_WEBSERVER_H

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include <upnp/upnp.h>

namespace upnp
{
    
class WebServer
{
public:
    WebServer(const std::string& webRoot);
    ~WebServer();
    
    void addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::string& data);
    void addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::vector<uint8_t>& data);
    void removeFile(const std::string& virtualDir, const std::string& filename);
    
    void clearFiles();
    
    void addVirtualDirectory(const std::string& virtualDirName);
    void removeVirtualDirectory(const std::string& virtualDirName);

    std::string getWebRootUrl();
    
private:
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

    static HostedFile& getFileFromRequest(const std::string& uri);
    
    static UpnpWebFileHandle openCallback(const char* pFilename, UpnpOpenFileMode mode);
    static int getInfoCallback(const char* pFilename, File_Info* pInfo);
    static int readCallback(UpnpWebFileHandle fileHandle, char* buf, size_t buflen);
    static int writeCallback(UpnpWebFileHandle fileHandle, char* buf, size_t buflen);
    static int seekCallback(UpnpWebFileHandle fileHandle, off_t offset, int origin);
    static int closeCallback(UpnpWebFileHandle fileHandle);
    
    std::string                                                         		m_WebRoot;
    static std::mutex                                                   		m_Mutex;
    static uint64_t																m_CurrentRequestId;
    static std::map<std::string, std::vector<std::unique_ptr<HostedFile>>>      m_ServedFiles;
    static std::vector<std::unique_ptr<FileHandle>>                     		m_OpenHandles;
};

}

#endif
