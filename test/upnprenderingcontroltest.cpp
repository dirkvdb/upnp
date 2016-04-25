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

#include "upnp.servicetestbase.h"
#include "upnp/upnp.avtransport.client.h"

using namespace utils;
using namespace testing;
using namespace std::placeholders;

#include "upnp/upnpxmlutils.h"

namespace upnp
{
namespace test
{

static const std::string g_controlUrl               = "ControlUrl";
static const std::string g_subscriptionUrl          = "SubscriptionUrl";
static const std::string g_serviceDescriptionUrl    = "ServiceDescriptionUrl";
static const std::string g_eventNameSpaceId         = "RCS";
static const uint32_t g_connectionId                = 0;
static const uint32_t g_defaultTimeout              = 1801;
static const Upnp_SID g_subscriptionId              = "subscriptionId";


class RenderingControlTest : public Test
{
public:
    virtual ~RenderingControlTest() {}

protected:
    void SetUp()
    {
        renderingControl = std::make_unique<RenderingControl::Client>(client);

        Service service;
        service.m_type                  = ServiceType::RenderingControl;
        service.m_controlURL            = g_controlUrl;
        service.m_eventSubscriptionURL  = g_subscriptionUrl;
        service.m_scpdUrl               = g_serviceDescriptionUrl;

        auto device = std::make_shared<Device>();
        device->m_type = DeviceType::MediaRenderer;
        device->m_services[service.m_type] = service;

        // set a valid device
        EXPECT_CALL(client, downloadXmlDocument(g_serviceDescriptionUrl))
            .WillOnce(Return(ixmlParseBuffer(testxmls::renderingServiceDescription.c_str())));
        renderingControl->setDevice(device);

        subscribe();

        Mock::VerifyAndClearExpectations(&client);
    }

    void TearDown()
    {
        Mock::VerifyAndClearExpectations(&client);
        unsubscribe();
        Mock::VerifyAndClearExpectations(&client);
    }

    void subscribe()
    {
        EXPECT_CALL(client, subscribeToService(g_subscriptionUrl, g_defaultTimeout, _))
            .WillOnce(Invoke([&] (const std::string&, int32_t, const std::shared_ptr<IServiceSubscriber>& cb) { callback = cb; }));

        renderingControl->StateVariableEvent.connect(std::bind(&EventListenerMock::RenderingControlLastChangedEvent, &eventListener, _1, _2), this);
        renderingControl->subscribe();

        Upnp_Event_Subscribe event;
        event.ErrCode = UPNP_E_SUCCESS;
        strcpy(event.PublisherUrl, g_subscriptionUrl.c_str());
        strcpy(event.Sid, g_subscriptionId);

        callback->onServiceEvent(UPNP_EVENT_SUBSCRIBE_COMPLETE, &event);
    }

    void unsubscribe()
    {
        EXPECT_CALL(client, unsubscribeFromService(callback));

        renderingControl->StateVariableEvent.disconnect(this);
        renderingControl->unsubscribe();
    }

    void triggerLastChangeUpdate(const std::string& mute, const std::string& volume)
    {
        std::vector<testxmls::EventValue> ev = { { "PresetNameList", "FactoryDefaults, InstallationDefaults" },
                                                 { "Mute", mute, {{ "channel", "master" }} },
                                                 { "Volume", volume, {{ "channel", "master" }} } };

        xml::Document doc(testxmls::generateStateVariableChangeEvent("LastChange", g_eventNameSpaceId, ev));

        Upnp_Event event;
        event.ChangedVariables = doc;
        strcpy(event.Sid, g_subscriptionId);

        client.UPnPEventOccurredEvent(&event);
    }

    std::unique_ptr<RenderingControl::Client>   renderingControl;
    StrictMock<ClientMock>                      client;
    StrictMock<EventListenerMock>               eventListener;
    std::shared_ptr<IServiceSubscriber>         callback;
};

TEST_F(RenderingControlTest, supportedActions)
{
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::GetVolume));
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::SetVolume));
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::ListPresets));
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::SelectPreset));
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::GetMute));
    EXPECT_TRUE(renderingControl->supportsAction(RenderingControl::Action::SetMute));

    EXPECT_FALSE(renderingControl->supportsAction(RenderingControl::Action::GetVolumeDB));
    EXPECT_FALSE(renderingControl->supportsAction(RenderingControl::Action::SetVolumeDB));
}

TEST_F(RenderingControlTest, lastChangeEvent)
{
    std::map<RenderingControl::Variable, std::string> lastChange;
    EXPECT_CALL(eventListener, RenderingControlLastChangedEvent(RenderingControl::Variable::LastChange, _)).WillOnce(SaveArg<1>(&lastChange));

    triggerLastChangeUpdate("0", "35");

    EXPECT_EQ("FactoryDefaults, InstallationDefaults", lastChange[RenderingControl::Variable::PresetNameList]);
    EXPECT_EQ("0", lastChange[RenderingControl::Variable::Mute]);
    EXPECT_EQ("35", lastChange[RenderingControl::Variable::Volume]);
}

TEST_F(RenderingControlTest, setVolume)
{
    EXPECT_CALL(eventListener, RenderingControlLastChangedEvent(RenderingControl::Variable::LastChange, _));
    triggerLastChangeUpdate("0", "35");

    // the service description xml defines the volume range between 10 and 110
    std::map<int32_t, std::string> values = { {69, "69"}, {120, "110"}, {0, "10"} };

    for (auto& value : values)
    {
        Action expectedAction("SetVolume", g_controlUrl, ServiceType::RenderingControl);
        expectedAction.addArgument("Channel", "Master");
        expectedAction.addArgument("DesiredVolume", value.second);
        expectedAction.addArgument("InstanceID", std::to_string(g_connectionId));

        EXPECT_CALL(client, sendAction(expectedAction))
            .WillOnce(Return(generateActionResponse(expectedAction.getName(), expectedAction.getServiceType())));

        renderingControl->setVolume(g_connectionId, value.first);
        Mock::VerifyAndClearExpectations(&client);
    }
}

}
}
