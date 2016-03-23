#pragma once

#include <cstdlib>
#include <system_error>

#include "uv.h"

namespace upnp
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

enum class RunMode : uint32_t
{
    Default = UV_RUN_DEFAULT,
    Once = UV_RUN_ONCE,
    NoWait = UV_RUN_NOWAIT
};

class DefaultLoop
{
public:
    DefaultLoop()
    {
    }

    void run()
    {
        checkRc(uv_run(uv_default_loop(), UV_RUN_DEFAULT));
    }

    uv_loop_t* get() noexcept
    {
        return uv_default_loop();
    }

    ~DefaultLoop() noexcept
    {
        uv_loop_close(uv_default_loop());
    }
};

class Loop
{
public:
    Loop(RunMode mode)
    {
        uv_loop_init(&m_loop);
        checkRc(uv_run(&m_loop, static_cast<uv_run_mode>(mode)));
    }

    uv_loop_t* get() noexcept
    {
        return &m_loop;
    }

    ~Loop() noexcept
    {
        uv_loop_close(&m_loop);
    }

private:
    uv_loop_t m_loop;
};

class Idler
{
public:
    template <typename LoopType, typename Callback>
    Idler(LoopType& loop, Callback&& cb)
    {
        checkRc(uv_idle_init(loop.get(), &m_idler));
        checkRc(uv_idle_start(&m_idler, cb));
    }

    ~Idler() noexcept
    {
        uv_idle_stop(&m_idler);
    }

private:
    uv_idle_t m_idler;
};

namespace socket
{

enum class UdpFlags : uint32_t
{
    Ipv6Only = UV_UDP_IPV6ONLY,
    Partial = UV_UDP_PARTIAL,
    ReuseAddress = UV_UDP_REUSEADDR
};

class Udp
{
public:
    template <typename LoopType>
    Udp(LoopType& loop, const std::string& ip, int32_t port, uint32_t flags)
    {
        checkRc(uv_udp_init(loop.get(), &m_socket));
        struct sockaddr_in addr;
        checkRc(uv_ip4_addr(ip.c_str(), port, &addr));
        checkRc(uv_udp_bind(&m_socket, reinterpret_cast<sockaddr*>(&addr), flags));
    }

    ~Udp() noexcept
    {
        close();
    }


    template <typename Callback>
    void read(Callback&& cb)
    {
        checkRc(uv_udp_recv_start(&m_socket, allocateBuffer, cb));
    }

    void close() noexcept
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_socket), nullptr);
    }

private:
    uv_udp_t    m_socket;
};

}

}