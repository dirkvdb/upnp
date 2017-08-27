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

struct EmptyState {};

template <typename T>
struct Task
{
    bool await_ready() { return false; }
    void await_suspend(std::experimental::coroutine_handle<> callerCoro)
    {
        utils::log::debug(__FUNCTION__);
        coro.promise().waiter = callerCoro;
        coro.resume();
    }

    T await_resume()
    {
        utils::log::debug(__FUNCTION__);
        assert(coro.promise().result.index() != 0);

        if (completionCb)
        {
            completionCb();
        }

        if (coro.promise().result.index() == 2)
        {
            std::rethrow_exception(std::get<2>(coro.promise().result));
        }

        return std::get<1>(coro.promise().result);
    }

//    void await_suspend(std::experimental::coroutine_handle<> coro)
//    {
//        func([this, coro] (std::error_code ec, T res) mutable {
//            if (ec)
//            {
//                result = ec;
//            }
//            else
//            {
//                result = std::move(res);
//            }
//            coro.resume();
//        });
//    }

    struct promise_type
    {
        std::variant<EmptyState, T, std::exception_ptr> result;
        std::experimental::coroutine_handle<> waiter;

        promise_type()
        {
            utils::log::debug(__FUNCTION__);
        }

        ~promise_type()
        {
            utils::log::debug(__FUNCTION__);
        }

        Task get_return_object()
        {
            utils::log::debug(__FUNCTION__);
            return { this };
        }

        std::experimental::suspend_always initial_suspend()
        {
            utils::log::debug(__FUNCTION__);
            return {};
        }

        auto final_suspend()
        {
            utils::log::debug(__FUNCTION__);
            struct Awaiter
            {
                promise_type* me;
                bool await_ready() { return false; }
                void await_suspend(std::experimental::coroutine_handle<>) { me->waiter.resume(); }
                void await_resume() {}
            };
            return Awaiter{ this };
        }

        template <typename U>
        void return_value(U&& value)
        {
            utils::log::debug(__FUNCTION__);
            result.template emplace<1>(value);
        }

        void unhandled_exception()
        {
            utils::log::debug(__FUNCTION__);
            result.template emplace<2>(std::current_exception());
        }
    };

    Task(promise_type* p)
    : coro(std::experimental::coroutine_handle<promise_type>::from_promise(*p))
    {
        utils::log::debug(__FUNCTION__);
    }

    ~Task()
    {
        utils::log::debug(__FUNCTION__);
        coro.destroy();
    }

    template <typename Callable>
    void setCompletionHandler(Callable&& cb)
    {
        completionCb = cb;
    }

    std::experimental::coroutine_handle<promise_type> coro;
    std::function<void()> completionCb;
};

template <>
struct Task<void>
{
    bool await_ready() { return false; }
    void await_suspend(std::experimental::coroutine_handle<> callerCoro)
    {
        coro.promise().waiter = callerCoro;
        coro.resume();
    }

    void await_resume()
    {
        if (completionCb)
        {
            completionCb();
        }

        assert(coro.promise().result.index() == 1);
        auto& res = std::get<1>(coro.promise().result);
        if (res)
        {
            std::rethrow_exception(res);
        }
    }

    struct promise_type
    {
        std::variant<EmptyState, std::exception_ptr> result;
        std::experimental::coroutine_handle<> waiter;

        Task get_return_object()
        {
            utils::log::debug(__FUNCTION__);
            return { this };
        }

        std::experimental::suspend_always initial_suspend()
        {
            return {};
        }

        auto final_suspend()
        {
            struct Awaiter
            {
                promise_type* me;
                bool await_ready() { return false; }
                void await_suspend(std::experimental::coroutine_handle<>) { me->waiter.resume(); }
                void await_resume() {}
            };
            return Awaiter{ this };
        }

        void return_void()
        {
            utils::log::debug(__FUNCTION__);
        }

        void unhandled_exception()
        {
            utils::log::debug(__FUNCTION__);
            result.emplace<1>(std::current_exception());
        }
    };

    Task(promise_type* p)
    : coro(std::experimental::coroutine_handle<promise_type>::from_promise(*p))
    {
    }

    ~Task()
    {
        coro.destroy();
    }

    template <typename Callable>
    void setCompletionHandler(Callable&& cb)
    {
        completionCb = cb;
    }

    std::experimental::coroutine_handle<promise_type> coro;
    std::function<void()> completionCb;
};

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

        std::experimental::suspend_always initial_suspend() { return {}; }
        auto final_suspend()
        {
            struct Awaiter
            {
                bool await_ready() { return false; }
                void await_resume() {}
                void await_suspend(std::experimental::coroutine_handle<promise_type> h)
                {
                    promise_type &me = h.promise();
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
            utils::log::info("connect Awaiter");
        }

        ~Awaiter()
        {
            utils::log::info("connect ~Awaiter");
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
            boost::beast::http::async_write(this->s, this->req, [this, coro] (auto ec) mutable {
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
            boost::beast::http::async_read(this->s, this->buf, this->res, [this, coro] (auto ec) mutable {
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
            boost::beast::http::async_read_header(this->s, this->buf, this->parser, [this, coro] (auto ec) mutable {
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
