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

#include "upnp.serviceclienttestbase.h"
#include "upnp/upnp.renderingcontrol.client.h"

using namespace utils;
using namespace testing;
using namespace std::placeholders;

#include "upnp/upnpxmlutils.h"

namespace upnp
{
namespace test
{

static const std::string g_eventNameSpaceId = "RCS";

struct RenderingControlStatusCallbackMock
{
    MOCK_METHOD1(onStatus, void(int32_t));
    MOCK_METHOD2(onStatus, void(int32_t, uint32_t volume));
};

class RenderingControlTest : public ServiceClientTestBase<RenderingControl::Client, RenderingControlStatusCallbackMock, RenderingControl::Variable>
{
public:
    RenderingControlTest() : ServiceClientTestBase(ServiceType::RenderingControl, testxmls::renderingServiceDescription)
    {
    }

    void triggerLastChangeUpdate(const std::string& mute, const std::string& volume)
    {
        std::vector<testxmls::EventValue> ev = { { "PresetNameList", "FactoryDefaults, InstallationDefaults" },
                                                 { "Mute", mute, {{ "channel", "master" }} },
                                                 { "Volume", volume, {{ "channel", "master" }} } };

        SubscriptionEvent event;
        event.sid  = g_subscriptionId;
        event.data = testxmls::generateStateVariableChangeEvent("LastChange", g_eventNameSpaceId, ev);
        eventCb(event);
    }
};

TEST_F(RenderingControlTest, supportedActions)
{
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::GetVolume));
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::SetVolume));
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::ListPresets));
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::SelectPreset));
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::GetMute));
    EXPECT_TRUE(serviceInstance->supportsAction(RenderingControl::Action::SetMute));

    EXPECT_FALSE(serviceInstance->supportsAction(RenderingControl::Action::GetVolumeDB));
    EXPECT_FALSE(serviceInstance->supportsAction(RenderingControl::Action::SetVolumeDB));
}

TEST_F(RenderingControlTest, lastChangeEvent)
{
    std::map<RenderingControl::Variable, std::string> lastChange;
    EXPECT_CALL(eventListener, LastChangedEvent(RenderingControl::Variable::LastChange, _)).WillOnce(SaveArg<1>(&lastChange));
    triggerLastChangeUpdate("0", "35");

    EXPECT_EQ("FactoryDefaults, InstallationDefaults", lastChange[RenderingControl::Variable::PresetNameList]);
    EXPECT_EQ("0", lastChange[RenderingControl::Variable::Mute]);
    EXPECT_EQ("35", lastChange[RenderingControl::Variable::Volume]);
}

TEST_F(RenderingControlTest, setVolume)
{
    EXPECT_CALL(eventListener, LastChangedEvent(RenderingControl::Variable::LastChange, _));
    triggerLastChangeUpdate("0", "35");

    // the service description xml defines the volume range between 10 and 110
    std::map<int32_t, std::string> values = { {69, "69"}, {120, "110"}, {0, "10"} };

    for (auto& value : values)
    {
        Action expectedAction("SetVolume", g_controlUrl, ServiceType::RenderingControl);
        expectedAction.addArgument("Channel", "Master");
        expectedAction.addArgument("DesiredVolume", value.second);
        expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));

        expectAction(expectedAction);
        EXPECT_CALL(statusMock, onStatus(200));
        serviceInstance->setVolume(g_connectionId, value.first, checkStatusCallback());
        Mock::VerifyAndClearExpectations(&client);
    }
}

TEST_F(RenderingControlTest, getVolume)
{
    Action expectedAction("GetVolume", g_controlUrl, ServiceType::RenderingControl);
    expectedAction.addArgument("Channel", "Master");
    expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));

    expectAction(expectedAction, { { "CurrentVolume", "36" } } );

    EXPECT_CALL(statusMock, onStatus(200, 36u));
    serviceInstance->getVolume(g_connectionId, checkStatusCallback<uint32_t>());
}

}
}
