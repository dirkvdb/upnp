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

#include <gtest/gtest.h>
#include <iostream>


using namespace utils;
using namespace testing;

#include "upnp/upnpcontrolpoint.h"
#include "upnp/upnpmediaserver.h"
#include "upnp/upnpmediarenderer.h"
#include "upnp/upnpitem.h"

#include "testenvironment.h"

extern upnp::test::TestEnvironment* g_Env;

namespace upnp
{
namespace test
{

class ControlPointTest : public Test
{
public:
    ControlPointTest()
    : m_Cp(g_Env->getClient())
    , m_Server(g_Env->getClient())
    , m_Renderer(g_Env->getClient())
    {
    }

protected:
    virtual void SetUp()
    {
        m_Server.setDevice(g_Env->getServer());
        m_Renderer.setDevice(g_Env->getRenderer());

        m_Cp.setRenderer(m_Renderer);
    }

    virtual void TearDown()
    {
    }

    ControlPoint                m_Cp;
    MediaServer                 m_Server;
    MediaRenderer               m_Renderer;
};

TEST_F(ControlPointTest, PlayItem)
{
    Resource res;
    res.setProtocolInfo(ProtocolInfo("http-get:*:audio/mpeg:DLNA.ORG_PN=MP3;DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000"));
    res.setUrl("http://192.168.1.100:8200/MediaItems/3916.mp3");

    Item item;
    item.addResource(res);

    //m_Cp.stop();
    //sleep(10);
    //m_Cp.playItem(m_Server, item);

    while (true)
        sleep(10);
}

}
}
