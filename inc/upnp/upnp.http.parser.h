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

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "upnp/upnp.flags.h"

namespace upnp
{
namespace http
{

enum class Type
{
    Request,
    Response,
    Both,
    EnumCount
};

enum class Method
{
    Notify,
    Search,
    Subscribe,
    Unsubscribe,
    Get,
    Head,
    Post,
    Unknown
};

struct Header
{
    Header(std::string f)
    : field(std::move(f))
    {
    }

    std::string field;
    std::string value;
};

class Parser
{
public:
    enum class Flag : uint32_t
    {
        Chunked             = 1 << 0,
        KeepAlive           = 1 << 1,
        ConnectionClose     = 1 << 2,
        ConnectionUpgrade   = 1 << 3,
        Trailing            = 1 << 4,
        Upgrade             = 1 << 5,
        SkipBody            = 1 << 6,
        ContentLength       = 1 << 7
    };

    Parser(Type type);
    ~Parser();

    void reset();

    void setHeadersCompletedCallback(std::function<void()> cb);
    void setCompletedCallback(std::function<void()> cb);
    void setBodyDataCallback(std::function<void(const char*, size_t)> cb);
    bool isCompleted();

    size_t parse(const char* data, size_t dataSize);
    size_t parse(const std::string& data);

    const std::string& headerValue(const char* name);
    Method getMethod() const noexcept;
    uint32_t getStatus() const noexcept;
    std::string stealBody() noexcept;
    std::string getUrl() noexcept;
    const std::vector<Header>& headers() const;
    Flags<Flag> getFlags() const noexcept;

    static const char* methodToString(Method m) noexcept;

    struct Range
    {
        uint64_t start = 0;
        uint64_t end = 0;
    };

    static Range parseRange(const std::string& range);

private:
    void clear();

    struct Pimpl;
    std::unique_ptr<Pimpl> m_pimpl;
};

}
}

