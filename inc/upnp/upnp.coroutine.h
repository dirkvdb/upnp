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

#include "utils/log.h"
#include "upnp/asio.h"

#include <variant>
#include <future>
#include <boost/beast/http.hpp>
#include <experimental/coroutine>

namespace upnp
{

namespace beast = boost::beast;

struct EmptyState {};

template <typename T>
struct MyTask
{
    struct promise_type
    {
        std::variant<T, std::exception_ptr> _value;
        bool _ready = false;
        std::experimental::coroutine_handle<> _waiter = nullptr;

        MyTask get_return_object()
        {
            return MyTask(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
        }

        auto initial_suspend()
        {
            return std::experimental::suspend_never();
        }

        auto final_suspend()
        {
            struct Awaiter
            {
                std::experimental::coroutine_handle<> _waiter = nullptr;

                bool await_ready() { return false; }

                void await_suspend(std::experimental::coroutine_handle<>)
                {
                    if (_waiter)
                    {
                        _waiter.resume();
                    }
                }

                void await_resume() {}
            };

            return Awaiter{_waiter};
        }

        void return_value(T value)
        {
            _value = value;
            _ready = true;
        }

        void unhandled_exception()
        {
            _value = std::current_exception();
            _ready = true;
        }
    };

    MyTask() = default;
    MyTask(const MyTask&) = delete;
    MyTask(MyTask&& f)
    : _coro(f._coro)
    {
        f._coro = nullptr;
    }

    MyTask& operator=(const MyTask&) = delete;
    MyTask& operator=(MyTask&& other)
    {
        _coro = other._coro;
        other._coro = nullptr;
        return *this;
    }

    explicit MyTask(std::experimental::coroutine_handle<promise_type> coro)
    : _coro(coro)
    {
    }

    ~MyTask()
    {
        if (_coro)
        {
            _coro.destroy();
        }
    }

    bool await_ready()
    {
        assert(_coro);
        return _coro.promise()._ready;
    }

    void await_suspend(std::experimental::coroutine_handle<> callerCoro)
    {
        _coro.promise()._waiter = callerCoro;
    }

    T await_resume()
    {
        return get();
    }

    T get()
    {
        if (std::holds_alternative<T>(_coro.promise()._value))
        {
            return std::get<T>(_coro.promise()._value);
        }

        std::rethrow_exception(std::get<std::exception_ptr>(_coro.promise()._value));
    }

    std::experimental::coroutine_handle<promise_type> _coro = nullptr;
};

template <>
struct MyTask<void>
{
    struct promise_type
    {
        bool _ready = false;
        std::exception_ptr _ex;
        std::experimental::coroutine_handle<> _waiter = nullptr;

        MyTask get_return_object()
        {
            return MyTask(std::experimental::coroutine_handle<promise_type>::from_promise(*this));
        }

        auto initial_suspend()
        {
            return std::experimental::suspend_never();
        }

        auto final_suspend()
        {
            struct Awaiter
            {
                std::experimental::coroutine_handle<> _waiter = nullptr;

                bool await_ready() { return false; }

                void await_suspend(std::experimental::coroutine_handle<>)
                {
                    if (_waiter)
                    {
                        _waiter.resume();
                    }
                }

                void await_resume() {}
            };

            return Awaiter{_waiter};
        }

        void return_void()
        {
            _ready = true;
        }

        void unhandled_exception()
        {
            _ex = std::current_exception();
            _ready = true;
        }
    };

    MyTask() = default;
    MyTask(const MyTask&) = delete;
    MyTask& operator=(const MyTask&) = delete;
    MyTask& operator=(MyTask&& other)
    {
        _coro = std::move(other._coro);
        other._coro = nullptr;
        return *this;
    }

    explicit MyTask(std::experimental::coroutine_handle<promise_type> coro)
    : _coro(coro)
    {
    }

    MyTask(MyTask&& f)
    : _coro(std::move(f._coro))
    {
        f._coro = nullptr;
    }

    ~MyTask()
    {
        if (_coro)
        {
            _coro.destroy();
        }
    }

    bool await_ready()
    {
        assert(_coro);
        return _coro.promise()._ready;
    }

    void await_suspend(std::experimental::coroutine_handle<> callerCoro)
    {
        _coro.promise()._waiter = callerCoro;
    }

    void await_resume()
    {
        get();
    }

    void get()
    {
        if (_coro.promise()._ex)
        {
            std::rethrow_exception(_coro.promise()._ex);
        }
    }

    std::experimental::coroutine_handle<promise_type> _coro = nullptr;
};

template <typename T>
using Future = MyTask<T>;

}

namespace upnp
{

inline auto async_connect(asio::ip::tcp::socket& s, const asio::ip::tcp::endpoint& ep)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& s;
        asio::ip::tcp::endpoint ep;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& s, const asio::ip::tcp::endpoint& ep)
        : s(s)
        , ep(ep)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            s.async_connect(this->ep, [this, coro] (auto ec) mutable {
                this->ready = true;
                this->ec = ec;
                assert(coro);
                coro.resume();
            });
        }
    };

    return Awaiter(s, ep);
}

inline auto async_accept(asio::ip::tcp::acceptor& acceptor, asio::ip::tcp::socket& socket)
{
    struct Awaiter
    {
        asio::ip::tcp::acceptor& acceptor;
        asio::ip::tcp::socket& socket;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::acceptor& acc, asio::ip::tcp::socket& sock)
        : acceptor(acc)
        , socket(sock)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            acceptor.async_accept(socket, [this, coro] (auto ec) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(acceptor, socket);
}

template <typename AsyncWriteStream>
inline auto async_write(asio::ip::tcp::socket& socket, const AsyncWriteStream& buf)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& socket;
        const AsyncWriteStream& buffer;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& sock, const AsyncWriteStream& buf)
        : socket(sock)
        , buffer(buf)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            asio::async_write(socket, buffer, [this, coro] (auto ec, size_t) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(socket, buf);
}

namespace http
{
inline auto async_write(asio::ip::tcp::socket& s, beast::http::request<beast::http::string_body>& req)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& s;
        beast::http::request<beast::http::string_body>& req;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& s, beast::http::request<beast::http::string_body>& req)
        : s(s)
        , req(req)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            utils::log::debug("http async write");
            boost::beast::http::async_write(this->s, this->req, [this, coro] (auto ec, size_t) mutable {
                utils::log::debug("http async written");
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(s, req);
}

template <class DynamicBuffer, bool isRequest, class Body, class Allocator>
inline auto async_read(asio::ip::tcp::socket& s,
                       DynamicBuffer& buf,
                       beast::http::message<isRequest, Body, boost::beast::http::basic_fields<Allocator>>& msg)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& s;
        beast::http::message<isRequest, Body, boost::beast::http::basic_fields<Allocator>>& msg;
        DynamicBuffer& buf;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& s, DynamicBuffer& buf, boost::beast::http::message<isRequest, Body, boost::beast::http::basic_fields<Allocator>>& msg)
        : s(s)
        , msg(msg)
        , buf(buf)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            boost::beast::http::async_read(this->s, this->buf, this->msg, [this, coro] (auto ec, size_t) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(s, buf, msg);
}

template <class DynamicBuffer>
inline auto async_read_header(asio::ip::tcp::socket& s, DynamicBuffer& buf, beast::http::parser<false, beast::http::empty_body>& parser)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& s;
        DynamicBuffer& buf;
        beast::http::parser<false, beast::http::empty_body>& parser;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& s, DynamicBuffer& buf, beast::http::parser<false, beast::http::empty_body>& parser)
        : s(s)
        , buf(buf)
        , parser(parser)
        {
        }

        bool await_ready()
        {
            return ready;
        }

        void await_resume()
        {
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        void await_suspend(std::experimental::coroutine_handle<> coro)
        {
            boost::beast::http::async_read_header(this->s, this->buf, this->parser, [this, coro] (auto ec, size_t) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(s, buf, parser);
}

}

}
