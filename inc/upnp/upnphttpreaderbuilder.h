//    Copyright (C) 2013 Dirk Vanden Boer <dirk.vdb@gmail.com>
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


#ifndef HTTP_READER_BUILDER_H
#define HTTP_READER_BUILDER_H

#include "upnp/upnphttpreader.h"
#include "audio/audioreaderfactory.h"

namespace doozy
{

class HttpReaderBuilder : public audio::IReaderBuilder
{
public:
    virtual bool supportsUri(const std::string& uri)
    {
        static const std::string http("http://");
    
        return uri.compare(0, http.length(), http) == 0;
    }
    
    virtual utils::IReader* build(const std::string& uri)
    {
        if (!supportsUri(uri))
        {
            throw std::logic_error("Uri is not supported by Http reader: " + uri);
        }
        
        return new upnp::HttpReader();
    }
};
    
}

#endif
