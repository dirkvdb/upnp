#ifndef UPNP_OBJECT_PRINTER_H
#define UPNP_OBJECT_PRINTER_H

#include "upnp/upnpaction.h"
#include <iostream>

namespace upnp
{

void PrintTo(const Action& action, std::ostream* os)
{
    *os << action.getActionDocument().toString();
}

}

#endif
