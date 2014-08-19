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

#include "utils/bufferedreader.h"

#include "upnp/upnpclient.h"
#include "upnp/upnpwebserver.h"
#include "upnp/upnphttpclient.h"
#include "upnp/upnphttpreader.h"
#include "upnp/upnpxmlutils.h"

using namespace utils;
using namespace testing;


namespace upnp
{
namespace test
{

class HttpReaderTest : public Test
{
public:
    HttpReaderTest()
    : httpClient(5)
    {
    }
    virtual ~HttpReaderTest() {}

protected:
    void SetUp()
    {
        client.initialize();
        webserver = std::make_unique<WebServer>("/");
    }
    
    void TearDown()
    {
        webserver.reset();
        client.destroy();
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

TEST_F(HttpReaderTest, downloadLargeBinaryFileBuffered)
{
    constexpr size_t fileSize = 1024 * 1024 * 1; //10MB
    std::vector<uint8_t> data(fileSize);
    
    webserver->addVirtualDirectory("virtualDir");
    webserver->addFile("virtualDir", "testfile.bin", "application/octet-stream", data);
    std::string url = webserver->getWebRootUrl() + "virtualDir/testfile.bin";
    
    BufferedReader reader(std::make_unique<HttpReader>(), 128 * 1024);
    reader.open(url);
    EXPECT_EQ(fileSize, reader.getContentLength());
    
    
    std::vector<uint8_t> result(fileSize, '\0');
    size_t currentPos = 0;
    size_t increment = 65536;
    
    do
    {
        auto bytesRead = reader.read(&result[currentPos], increment);
        currentPos += bytesRead;
    }
    while (!reader.eof());
    
    EXPECT_EQ(0, memcmp(data.data(), result.data(), data.size()));
}

}
}
