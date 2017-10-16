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

#include <stdexcept>

#include "upnp/upnp.coroutine.h"

namespace upnp::test {
#define EXPECT_SYSTEM_ERROR(call, errorCode) \
    try \
    { \
        call; \
        FAIL() << "Should have thrown an exception"; \
    } \
    catch (const std::system_error& e) \
    { \
        EXPECT_EQ(errorCode, e.code()); \
    } \
    catch (...) \
    { \
        FAIL() << "Unexpected exception thrown"; \
    }
}

namespace upnp::test
{

template <typename TaskResult>
auto runCoroTask(asio::io_service& io, Future<TaskResult>&& task)
{
    while (!task.await_ready())
    {
        io.run_one();
    }

    return task.get();
}

}
