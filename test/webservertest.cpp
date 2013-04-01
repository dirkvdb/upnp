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

#include "utils/signal.h"
#include "gtest/gtest.h"

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
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(1, result[0]);
    EXPECT_EQ(2, result[1]);
    
    // read byte 5 and 6
    result = httpClient.getData(url, 4, 2);
    
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(5, result[0]);
    EXPECT_EQ(6, result[1]);
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
    EXPECT_EQ(0, reader.currentPosition());
    
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
