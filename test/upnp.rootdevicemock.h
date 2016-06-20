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

#pragma once

#include <asio.hpp>

#include "gmock/gmock.h"
#include "upnp/upnp.rootdeviceinterface.h"

using namespace testing;

namespace upnp
{
namespace test
{

struct RootDeviceMock : public IRootDevice
{
    MOCK_METHOD0(initialize, void());
    MOCK_METHOD0(uninitialize, void());

    MOCK_METHOD0(getWebrootUrl, std::string());
    MOCK_METHOD2(registerDevice, void(const std::string& deviceDescriptionXml, const Device& dev));

    MOCK_METHOD0(getUniqueDeviceName, std::string());

    MOCK_METHOD2(notifyEvent, void(const std::string& serviceId, std::string response));
    MOCK_METHOD3(addFileToHttpServer, void(const std::string& path, const std::string& contentType, const std::string& data));
    MOCK_METHOD3(addFileToHttpServer, void(const std::string& path, const std::string& contentType, const std::vector<uint8_t>& data));
    MOCK_METHOD1(removeFileFromHttpServer, void(const std::string& path));
    
    asio::io_service& ioService() noexcept override
    {
        return m_io;
    }
        
private:
    asio::io_service m_io;
};

}
}
