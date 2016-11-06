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

#ifdef STD_ASIO
    #include <asio.hpp>
    #include <asio/steady_timer.hpp>
    #include <system_error>

    namespace upnp
    {
        using asio_error_code = asio::error_code;
    }
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #include <boost/asio.hpp>
    #include <boost/asio/steady_timer.hpp>
    #include <boost/system/error_code.hpp>
    #pragma GCC diagnostic pop

    namespace asio = boost::asio;

    namespace upnp
    {
        using asio_error_code = boost::system::error_code;
    }
#endif
