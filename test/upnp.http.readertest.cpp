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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "utils/bufferedreader.h"

#include "upnp/upnp.uv.h"
#include "upnp/upnp.http.reader.h"
#include "upnp/upnp.http.server.h"

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
    : webserver(io)
    {
        webserver.start(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 0));
    }

protected:
    asio::io_service io;
    http::Server webserver;
};

TEST_F(HttpReaderTest, downloadLargeBinaryFileBuffered)
{
    constexpr size_t fileSize = 1024 * 1024 * 1; //10MB
    std::vector<uint8_t> data(fileSize);

    webserver.addFile("/testfile.bin", "application/octet-stream", data);

    auto fut = std::async(std::launch::async, [=] () {
        std::string url = webserver.getWebRootUrl() + "/testfile.bin";

        BufferedReader reader(std::make_unique<http::Reader>(), 128 * 1024);
        reader.open(url);
        EXPECT_EQ(fileSize, reader.getContentLength());

        std::vector<uint8_t> result(fileSize);
        size_t currentPos = 0;
        size_t increment = 65536;

        uint64_t bytesRead = 0;

        do
        {
            EXPECT_NO_THROW(bytesRead = reader.read(&result[currentPos], increment));
            currentPos += bytesRead;
        }
        while (!reader.eof() && bytesRead > 0);

        EXPECT_THAT(data, ContainerEq(result));
        EXPECT_EQ(0, memcmp(data.data(), result.data(), data.size()));

        io.post([this] () {
            webserver.stop();
        });
    });

    io.run();
    fut.wait();
}

}
}
