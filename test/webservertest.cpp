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

#include "utils/log.h"
#include "utils/signal.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "upnp/upnpclient.h"
#include "upnp/upnpwebserver.h"
#include "upnp/upnphttpclient.h"
#include "upnp/upnphttpreader.h"


using namespace utils;
using namespace testing;

#include "upnp/upnpxmlutils.h"

namespace upnp
{
namespace test
{

class VirtualDirCallback : public IVirtualDirCallback
{
public:
    MOCK_METHOD2(read, uint64_t(uint8_t*, uint64_t));
    MOCK_METHOD1(seekRelative, void(uint64_t));
    MOCK_METHOD1(seekAbsolute, void(uint64_t));
    MOCK_METHOD1(seekFromEnd, void(uint64_t));
    MOCK_METHOD0(close, void());
};

class WebServerTest : public Test
{
public:
    WebServerTest()
    : httpClient(5)
    {
    }
    virtual ~WebServerTest() {}

protected:
    void SetUp()
    {
        client.initialize();
        webserver.reset(new WebServer("/"));
    }
    
    void TearDown()
    {
        webserver.reset();
        client.destroy();
    }
    
    std::string createTextFile()
    {
        std::stringstream ss;
        for (int32_t i = 0; i < 10000; ++i)
        {
            ss << i << ";";
        }
        
        return ss.str();
    }
    
    std::vector<uint8_t> createBinaryFile()
    {
        std::vector<uint8_t> data;
        for (int32_t i = 0; i < 10000; ++i)
        {
            data.push_back(i + 1);
        }
        
        return data;
    }
    
    Client                      client;
    HttpClient                  httpClient;
    std::unique_ptr<WebServer>  webserver;
};

TEST_F(WebServerTest, downloadTextFile)
{
    auto file = createTextFile();
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.txt", "text/plain", file);
    
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.txt";
    
    EXPECT_EQ(file.size(), httpClient.getContentLength(url));
    EXPECT_EQ(file, httpClient.getText(url));
}

TEST_F(WebServerTest, downloadBinaryFile)
{
    auto file = createBinaryFile();
    
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.bin", "application/octet-stream", file);
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.bin";
    
    auto result = httpClient.getData(url);
    
    EXPECT_EQ(file.size(), httpClient.getContentLength(url));
    EXPECT_EQ(0, memcmp(file.data(), result.data(), file.size()));
}

TEST_F(WebServerTest, downloadPartialBinaryFile)
{
    auto file = createBinaryFile();
    
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.bin", "application/octet-stream", file);
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.bin";
    
    // read byte 1 and 2
    auto result = httpClient.getData(url, 0, 2);
    EXPECT_EQ(2U, result.size());
    EXPECT_EQ(1U, result[0]);
    EXPECT_EQ(2U, result[1]);
    
    // read byte 5 and 6
    result = httpClient.getData(url, 4, 2);
    
    EXPECT_EQ(2U, result.size());
    EXPECT_EQ(5U, result[0]);
    EXPECT_EQ(6U, result[1]);
}


TEST_F(WebServerTest, downloadTextFileThroughCalllback)
{
    auto cb = std::make_shared<StrictMock<VirtualDirCallback>>();
    auto file = createTextFile();
    auto filePath = "virtualDir/?id=@100";
    auto requestedFilePath = stringops::format("/%s", filePath);
    
    fileops::FileSystemEntryInfo info;
    info.modifyTime = 200;
    info.sizeInBytes = file.size();
    info.type = fileops::FileSystemEntryType::File;
    
    auto fileInfoCb = [&] (const std::string& path) {
        EXPECT_EQ(requestedFilePath, path);
        return info;
    };
    auto requestCb = [&] (const std::string& path) {
        EXPECT_EQ(requestedFilePath, path);
        return cb;
    };
    
    webserver->addVirtualDirectory("virtualDir", fileInfoCb, requestCb);
    
    EXPECT_CALL(*cb, read(_, file.size())).WillRepeatedly(Invoke([&] (uint8_t* pData, int32_t size) -> int32_t {
        memcpy(pData, file.data(), size);
        return size;
    }));
    EXPECT_CALL(*cb, close());
    
    std::string url = webserver->getWebRootUrl() + filePath;
    EXPECT_EQ(file, httpClient.getText(url));
    
    // we should have the last reference to the mock now, the server should have released the callback
    EXPECT_TRUE(cb.unique());
}

TEST_F(WebServerTest, downloadBinaryFileThroughCalllback)
{
//    auto file = createBinaryFile();
//    
//    webserver->addVirtualDirectory("virtualDir");
//    webserver->addFile("virtualDir", "testfile.bin", "application/octet-stream", file);
//    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.bin";
//    
//    auto result = httpClient.getData(url);
//    
//    EXPECT_EQ(file.size(), httpClient.getContentLength(url));
//    EXPECT_EQ(0, memcmp(file.data(), result.data(), file.size()));
}


TEST_F(WebServerTest, downloadBinaryFileUsingHttpReader)
{
    auto file = createBinaryFile();
    
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.bin", "application/octet-stream", file);
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.bin";
    
    HttpReader reader;
    EXPECT_THROW(reader.getContentLength(), std::logic_error);
    EXPECT_THROW(reader.currentPosition(), std::logic_error);
    
    reader.open(url);
    EXPECT_EQ(file.size(), reader.getContentLength());
    EXPECT_EQ(0U, reader.currentPosition());
    
    uint64_t size = reader.getContentLength();
    std::vector<uint8_t> result(size, '\0');
    
    // read the file in blocks of 1kb
    for (uint64_t i = 0; i + 1 < size; i += 1024)
    {
        uint64_t reqSize = std::min<uint64_t>(size - i, 1024);
        reader.read(result.data() + i, reqSize);
    }
    
    EXPECT_EQ(0, memcmp(file.data(), result.data(), file.size()));
}

TEST_F(WebServerTest, addRemoveVirtualDir)
{
    std::string contents = "testfilecontents";
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.txt";
    
    EXPECT_THROW(httpClient.getContentLength(url), std::logic_error);
    
    webserver->addVirtualDirectory("virtualDir");
    EXPECT_THROW(httpClient.getContentLength(url), std::logic_error);
    
    webserver->addFile("virtualDir", "testfile.txt", "text/plain", contents);
    EXPECT_NO_THROW(httpClient.getContentLength(url));
    
    webserver->removeVirtualDirectory("virtualDir");
    EXPECT_THROW(httpClient.getContentLength(url), std::logic_error);
}

TEST_F(WebServerTest, restartServer)
{
    auto file = createTextFile();
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.txt", "text/plain", file);
    
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.txt";

    EXPECT_EQ(file.size(), httpClient.getContentLength(url));
    EXPECT_EQ(file, httpClient.getText(url));

    webserver.reset();
    client.destroy();
    client.initialize();
    webserver.reset(new WebServer("/"));

    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.txt", "text/plain", file);
    
    url = webserver->getWebRootUrl() + "virtualDir/testfile.txt";

    EXPECT_EQ(file.size(), httpClient.getContentLength(url));
    EXPECT_EQ(file, httpClient.getText(url));
}

}
}
