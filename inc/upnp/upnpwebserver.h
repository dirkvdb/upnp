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

#include <string>
#include <vector>
#include <functional>

#include "utils/fileoperations.h"

namespace upnp
{

class IVirtualDirCallback
{
public:
    virtual ~IVirtualDirCallback() {}

    virtual uint64_t read(uint8_t* buf, uint64_t buflen) = 0;
    virtual void seekAbsolute(uint64_t position) = 0;
    virtual void seekRelative(uint64_t offset) = 0;
    virtual void seekFromEnd(uint64_t offset) = 0;
    virtual void close() = 0;
};

using FileInfoCb = std::function<utils::fileops::FileSystemEntryInfo(const std::string&)>;
using RequestCb = std::function<std::shared_ptr<IVirtualDirCallback>(const std::string&)>;
    
class WebServer
{
public:
    WebServer(const std::string& webRoot);
    ~WebServer();
    
    void addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::string& data);
    void addFile(const std::string& virtualDir, const std::string& filename, const std::string& contentType, const std::vector<uint8_t>& data);
    void removeFile(const std::string& virtualDir, const std::string& filename);
    
    void clearFiles();
    
    // adds a virtual directory, in memory files can be added using addFile
    void addVirtualDirectory(const std::string& virtualDirName);
    // adds a virtual directory, the callback is called on incoming requests in this directory
    void addVirtualDirectory(const std::string& virtualDirName, FileInfoCb fileinfoCb, RequestCb requestCb);
    void removeVirtualDirectory(const std::string& virtualDirName);

    std::string getWebRootUrl();
    
private:
    std::string m_webRoot;
};

}

#endif
