#pragma once

#include <string>

namespace upnp
{

class Device;

namespace xml
{
    void parseDeviceInfo(const std::string& xml, Device& device);
}
}

