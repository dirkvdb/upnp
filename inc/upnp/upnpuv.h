#pragma once

#include <chrono>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <system_error>

#include <uv.h>

namespace uv
{

namespace
{

void checkRc(int rc)
{
    if (rc < 0)
    {
        throw std::system_error(rc, std::system_category(), uv_strerror(rc));
    }
}

void allocateBuffer(uv_handle_t* /*handle*/, size_t suggested_size, uv_buf_t* buf)
{
    *buf = uv_buf_init(reinterpret_cast<char*>(std::malloc(suggested_size)), suggested_size);
}

}

template<typename T, typename ValueType = typename std::underlying_type<T>::type>
class Flags
{
public:
    Flags() = default;

    Flags(T flag)
    : m_flags(getValue(flag))
    {
    }

    template <typename... Flag>
    Flags(Flag&&... flags)
    : m_flags(getValue(std::forward<Flag&&>(flags)...))
    {
    }

    operator ValueType()
    {
        return m_flags;
    }

private:
    ValueType getValue(T flag)
    {
        return static_cast<ValueType>(flag);
    }

    template <typename... Flag>
    ValueType getValue(T flag, Flag&&... flags)
    {
        return getValue(flag) | getValue(std::forward<T>(flags)...);
    }

    ValueType m_flags = 0;
};

enum class RunMode : uint32_t
{
    Default = UV_RUN_DEFAULT,
    Once = UV_RUN_ONCE,
    NoWait = UV_RUN_NOWAIT
};

class Loop
{
public:
    Loop()
    {
        checkRc(uv_loop_init(&m_handle));
        m_handle.data = this;
    }

    uv_loop_t* get() noexcept
    {
        return &m_handle;
    }

    void run(RunMode mode)
    {
        checkRc(uv_run(&m_handle, static_cast<uv_run_mode>(mode)));
    }

    void stop()
    {
        uv_stop(&m_handle);
    }

private:
    uv_loop_t  m_handle;
};

template <typename HandleType>
class Handle
{
public:
    bool isClosing()
    {
        return uv_is_closing(get()) != 0;
    }

    void close(std::function<void()> cb)
    {
        m_closeCallback = std::move(cb);
        uv_close(reinterpret_cast<uv_handle_t*>(get()), [] (auto* handle) {
            reinterpret_cast<Handle<HandleType>*>(handle->data)->m_closeCallback();
        });
    }

protected:
    template <typename InitFunc>
    void init(Loop& loop, InitFunc func)
    {
        checkRc(func(loop.get(), &m_handle));
        m_handle.data = this;
    }

    HandleType* get()
    {
        return &m_handle;
    }

private:
    std::function<void()>   m_closeCallback;
    HandleType              m_handle;
};

class Idler : private Handle<uv_idle_t>
{
public:
    template <typename Callback>
    Idler(Loop& loop, Callback&& cb)
    : m_callback(cb)
    {
        init(loop, uv_idle_init);
        checkRc(uv_idle_start(get(), [] (auto* handle) {
            reinterpret_cast<Idler*>(handle->data)->m_callback();
        }));
    }

    ~Idler() noexcept
    {
        uv_idle_stop(get());
    }

private:
    std::function<void()>   m_callback;
};

class Signal : private Handle<uv_signal_t>
{
public:
    Signal(Loop& loop)
    {
        init(loop, uv_signal_init);
    }

    void start(std::function<void()> cb, int32_t signalNumber)
    {
        m_callback = std::move(cb);
        checkRc(uv_signal_start(get(), [] (auto* handle, int /*signum*/) {
            reinterpret_cast<Signal*>(handle->data)->m_callback();
        }, signalNumber));
    }

    void stop() noexcept
    {
        uv_signal_stop(get());
    }

private:
    std::function<void()>   m_callback;
};

class Timer : private Handle<uv_timer_t>
{
public:
    Timer(Loop& loop)
    {
        init(loop, uv_timer_init);
    }

    void start(std::chrono::milliseconds timeout, std::chrono::milliseconds repeat, std::function<void()> cb)
    {
        m_callback = std::move(cb);
        checkRc(uv_timer_start(get(), [] (auto* handle) {
            reinterpret_cast<Timer*>(handle->data)->m_callback();
        }, timeout.count(), repeat.count()));
    }

    void again()
    {
        checkRc(uv_timer_again(get()));
    }

    void stop()
    {
        checkRc(uv_timer_stop(get()));
    }

private:
    std::function<void()>   m_callback;
};

namespace socket
{

enum class UdpFlag : uint32_t
{
    Ipv6Only = UV_UDP_IPV6ONLY,
    Partial = UV_UDP_PARTIAL,
    ReuseAddress = UV_UDP_REUSEADDR
};

template <typename T>
inline Flags<T> operator | (T lhs, T rhs)
{
    return Flags<T>(lhs, rhs);
}

class Udp : private Handle<uv_udp_t>
{
public:
    Udp(Loop& loop)
    {
        init(loop, uv_udp_init);
    }

    void bind(const std::string& ip, int32_t port, Flags<UdpFlag> flags = uv::Flags<UdpFlag>())
    {
        struct sockaddr_in addr;
        checkRc(uv_ip4_addr(ip.c_str(), port, &addr));
        checkRc(uv_udp_bind(get(), reinterpret_cast<sockaddr*>(&addr), flags));
    }

    template <typename Callback>
    void read(Callback&& cb)
    {
        checkRc(uv_udp_recv_start(get(), allocateBuffer, cb));
    }

    void close() noexcept
    {
        Handle<uv_udp_t>::close([] () {});
    }
};

}

inline void stopLoopAndCloseRequests(Loop& loop)
{
    loop.stop();
    uv_walk(loop.get(), [] (uv_handle_t* handle, void* /*arg*/) {
        auto* handleInstance = reinterpret_cast<Handle<uv_handle_t>*>(handle->data);
        assert(handleInstance);
        if (!handleInstance->isClosing())
        {
            handleInstance->close([] () { std::cout << "closed" << std::endl; });
        }
    }, nullptr);
}

}