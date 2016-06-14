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

#include "gmock/gmock.h"

#include "upnp/upnp.action.h"
#include "upnp/upnp.renderingcontrol.service.h"
#include "upnp.renderingcontrol.typeconversions.h"
#include "upnp.rootdevicemock.h"

namespace upnp
{
namespace test
{

using namespace testing;
using namespace std::string_literals;

static const std::string s_controlUrl = "http://controlurl:80";
static const uint32_t s_connectionId = 5;

struct RenderingControlMock : public IRenderingControl
{
    MOCK_METHOD2(selectPreset, void(uint32_t instanceId, const std::string& name));
    MOCK_METHOD3(setVolume, void(uint32_t instanceId, RenderingControl::Channel, uint16_t volume));
};

class RenderingControlServiceTest : public Test
{
public:
    RenderingControlServiceTest()
    : service(rootDeviceMock, serviceImplMock)
    {
        rootDeviceMock.ControlActionRequested = [this] (auto& request) {
            return service.onAction(request.actionName, request.action).toString();
        };
    }

    Action createAction(RenderingControl::Action type)
    {
        Action a(RenderingControl::toString(type), s_controlUrl, serviceType);
        a.addArgument("InstanceID", std::to_string(s_connectionId));
        return a;
    }

    ActionRequest createActionRequest(RenderingControl::Action type, const Action& a)
    {
        ActionRequest req;
        req.serviceType = serviceTypeToUrnIdString(serviceType);
        req.actionName = RenderingControl::toString(type);
        req.action = a.toString();
        return req;
    }

    ServiceType serviceType = ServiceType{ServiceType::RenderingControl, 1};
    RootDeviceMock rootDeviceMock;
    RenderingControlMock serviceImplMock;
    RenderingControl::Service service;
};

TEST_F(RenderingControlServiceTest, SetVolume)
{
    auto a = createAction(RenderingControl::Action::SetVolume);
    a.addArgument("Channel", "Master");
    a.addArgument("DesiredVolume", "45");

    EXPECT_CALL(serviceImplMock, setVolume(s_connectionId, RenderingControl::Channel::Master, 45));
    rootDeviceMock.ControlActionRequested(createActionRequest(RenderingControl::Action::SetVolume, a));
}

TEST_F(RenderingControlServiceTest, GetVolume)
{
    EXPECT_CALL(rootDeviceMock, notifyEvent(_, _)).Times(AnyNumber());
    service.setVolume(s_connectionId, RenderingControl::Channel::Master, 54);

    auto a = createAction(RenderingControl::Action::GetVolume);
    a.addArgument("Channel", "Master");

    auto response = rootDeviceMock.ControlActionRequested(createActionRequest(RenderingControl::Action::GetVolume, a));

    ActionResponse expected(RenderingControl::toString(RenderingControl::Action::GetVolume), serviceType);
    expected.addArgument("CurrentVolume", "54");
    EXPECT_EQ(expected.toString(), response);
}

}
}
