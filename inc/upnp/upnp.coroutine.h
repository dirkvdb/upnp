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

struct sync_await_helper
{
    struct promise_type
    {
        std::condition_variable c;
        std::mutex m;
        bool done = false;

        sync_await_helper get_return_object()
        {
            return {std::experimental::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::experimental::suspend_never initial_suspend() { return {}; }
        auto final_suspend()
        {
            struct Awaiter
            {
                bool await_ready() { return false; }
                void await_resume() {}
                void await_suspend(std::experimental::coroutine_handle<promise_type> h)
                {
                    utils::log::debug("Connect done");
                    promise_type& me = h.promise();
                    {
                        std::lock_guard<std::mutex> lock(me.m);
                        me.done = true;
                    }

                    me.c.notify_all();
                }
            };

            return Awaiter{};
        }

        void unhandled_exception()
        {
            std::rethrow_exception(std::current_exception());
        }

        void return_void() {}
    };

    ~sync_await_helper() { handle.destroy(); }
    sync_await_helper(std::experimental::coroutine_handle<promise_type> h) : handle(h) {}
    std::experimental::coroutine_handle<promise_type> handle;
};

template <typename Awaitable>
auto sync_await(Awaitable&& a)
{
    if (!a.await_ready())
    {
        auto coro = []() -> sync_await_helper { co_return; }();
        a.await_suspend(coro.handle);

        auto *p = &coro.handle.promise();
        std::unique_lock<std::mutex> lock(p->m);
        p->c.wait(lock, [p] { return p->done; });
    }

    return a.await_resume();
}

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
                coro.resume();
            });
        }
    };

    return Awaiter(s, ep);
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
            boost::beast::http::async_write(this->s, this->req, [this, coro] (auto ec, size_t) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(s, req);
}

template <class DynamicBuffer>
inline auto async_read(asio::ip::tcp::socket& s, DynamicBuffer& buf, beast::http::response<beast::http::string_body>& res)
{
    struct Awaiter
    {
        asio::ip::tcp::socket& s;
        beast::http::response<beast::http::string_body>& res;
        DynamicBuffer& buf;
        asio_error_code ec;
        bool ready = false;

        Awaiter(asio::ip::tcp::socket& s, DynamicBuffer& buf, beast::http::response<beast::http::string_body>& res)
        : s(s)
        , res(res)
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
            boost::beast::http::async_read(this->s, this->buf, this->res, [this, coro] (auto ec, size_t) mutable {
                this->ready = true;
                this->ec = ec;
                coro.resume();
            });
        }
    };

    return Awaiter(s, buf, res);
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
