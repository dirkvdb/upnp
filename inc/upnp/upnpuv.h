#pragma once

#include <array>
#include <string>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <system_error>

#include <uv.h>
#include "utils/log.h"

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

class Buffer
{
public:
    Buffer(uint8_t* data, size_t size)
    : m_handle(uv_buf_init(reinterpret_cast<char*>(data), size))
    {
    }

    template <typename T>
    Buffer(T& data)
    : m_handle(uv_buf_init(reinterpret_cast<char*>(data.data()), data.size()))
    {
    }

    uv_buf_t* get()
    {
        return &m_handle;
    }

    const uv_buf_t* get() const
    {
        return &m_handle;
    }

private:
    uv_buf_t m_handle;
};

template<typename T, typename ValueType = typename std::underlying_type<T>::type>
class Flags
{
public:
    Flags() = default;
    Flags(const Flags<T, ValueType>& other) = default;
    Flags(Flags<T, ValueType>&& other) = default;

    Flags(T flag)
    : m_flags(getValue(flag))
    {
    }
    
    Flags(ValueType flags)
    : m_flags(flags)
    {
    }

    template <typename... Flag>
    Flags(Flag&&... flags)
    : m_flags(getValue(std::forward<Flag&&>(flags)...))
    {
    }

    operator ValueType() const noexcept
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
    
template <typename T>
inline Flags<T> operator | (T lhs, T rhs)
{
    return Flags<T>(lhs, rhs);
}

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

    HandleType* get() noexcept
    {
        return &m_handle;
    }

protected:
    Handle() = default;
    ~Handle() = default;

    Handle(HandleType handle)
    : m_handle(handle)
    {
    }

    template <typename InitFunc>
    void init(Loop& loop, InitFunc func)
    {
        checkRc(func(loop.get(), &m_handle));
        m_handle.data = this;
    }
    
    template <typename InitFunc, typename Arg>
    void init(Loop& loop, InitFunc func, Arg&& arg)
    {
        checkRc(func(loop.get(), &m_handle, arg));
        m_handle.data = this;
    }

private:
    std::function<void()>   m_closeCallback;
    HandleType              m_handle;
};

template <typename HandleType>
class Stream : public Handle<HandleType>
{
public:
    void read(std::function<void(std::string)> cb)
    {
        m_readCb = std::move(cb);
        checkRc(uv_read_start(this->get(), allocateBuffer, [] (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
            auto* thisPtr = reinterpret_cast<Stream*>(stream->data);
            thisPtr->m_readCb(std::string(buf->base, nread));
        }));
    }

    void stopRead()
    {
        checkRc(uv_read_stop(this->get()));
    }

    void write(const Buffer& buf, std::function<void(int32_t)> cb)
    {
        auto write = std::make_unique<uv_write_t>();
        write->data = new std::function<void(int32_t)>(cb);
        checkRc(uv_write(write.release(), this->get(), buf.get(), 1, [] (uv_write_t* req, int status) {
            std::unique_ptr<uv_write_t> writePtr(req);
            std::unique_ptr<std::function<void(int32_t)>> cb(reinterpret_cast<std::function<void(int32_t)>*>(writePtr->data));
            (*cb)(status);
        }));
    }

private:
    std::function<void(std::string)> m_readCb;
};

class Idler : public Handle<uv_idle_t>
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

class Signal : public Handle<uv_signal_t>
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
    
enum class PollEvent : int32_t
{
    Readable = UV_READABLE,
    Writable = UV_WRITABLE
    // Version 1.9.0
    //Disconnect = UV_DISCONNECT
};
    
struct OsSocket
{
    OsSocket(uv_os_sock_t sock)
    : m_sock(sock)
    {
    }
    
    operator uv_os_sock_t() noexcept
    {
        return m_sock;
    }
    
private:
    uv_os_sock_t m_sock;
};
    
class Poll : public Handle<uv_poll_t>
{
public:
    Poll(Loop& loop, int32_t fd)
    {
        init(loop, uv_poll_init, fd);
    }
    
    Poll(Loop& loop, OsSocket sock)
    {
        init(loop, uv_poll_init_socket, sock);
    }
    
    void start(Flags<PollEvent> events, std::function<void(int32_t, Flags<PollEvent>)> cb)
    {
        m_callback = std::move(cb);
        checkRc(uv_poll_start(get(), events, [] (auto* handle, int status, int events) {
            reinterpret_cast<Poll*>(handle->data)->m_callback(status, events);
        }));
    }
    
    void stop() noexcept
    {
        uv_poll_stop(get());
    }
    
private:
    std::function<void(int32_t, Flags<PollEvent>)> m_callback;
};

class Connection : public Handle<uv_connect_t>
{
public:
    Connection() = default;
};

class Timer : public Handle<uv_timer_t>
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

class Address
{
public:
    static Address createIp4(const std::string& ip, int32_t port)
    {
        Address addr;
        checkRc(uv_ip4_addr(ip.c_str(), port, &addr.m_address.address4));
        return addr;
    }

    static Address createIp6(const std::string& ip, int32_t port)
    {
        Address addr;
        checkRc(uv_ip6_addr(ip.c_str(), port, &addr.m_address.address6));
        return addr;
    }

    bool isIpv4() const noexcept
    {
        return m_address.address4.sin_family == AF_INET;
    }

    bool isIpv6() const noexcept
    {
        return m_address.address4.sin_family == AF_INET6;
    }

    const sockaddr_in* get() const noexcept
    {
        return &m_address.address4;
    }

private:
    Address() = default;

    union
    {
        struct sockaddr_in address4;
        struct sockaddr_in6 address6;
    } m_address;
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

class Udp : public Handle<uv_udp_t>
{
public:
    enum class MemberShip : uint32_t
    {
        LeaveGroup = UV_LEAVE_GROUP,
        JoinGroup = UV_JOIN_GROUP
    };

    Udp(Loop& loop)
    {
        init(loop, uv_udp_init);
    }

    void bind(const Address& addr, Flags<UdpFlag> flags = uv::Flags<UdpFlag>())
    {
        checkRc(uv_udp_bind(get(), reinterpret_cast<const sockaddr*>(addr.get()), flags));
    }

    void setMemberShip(const std::string& ip, MemberShip memberShip)
    {
        checkRc(uv_udp_set_membership(get(), ip.c_str(), nullptr, static_cast<uv_membership>(memberShip)));
    }

    void setMulticastLoop(bool enabled)
    {
        checkRc(uv_udp_set_multicast_loop(get(), enabled ? 1 : 0));
    }

    void setMulticastTtl(int32_t ttl)
    {
        checkRc(uv_udp_set_multicast_ttl(get(), ttl));
    }

    void setMulticastInterface(const std::string& itf)
    {
        checkRc(uv_udp_set_multicast_interface(get(), itf.c_str()));
    }

    void setBroadcast(bool enabled)
    {
        checkRc(uv_udp_set_broadcast(get(), enabled ? 1 : 0));
    }

    void setTtl(int32_t ttl)
    {
        checkRc(uv_udp_set_ttl(get(), ttl));
    }

    void recv(std::function<void(std::string)> cb)
    {
        m_recvCb = std::move(cb);

        checkRc(uv_udp_recv_start(get(), allocateBuffer, [] (uv_udp_t* req, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* /*addr*/, unsigned /*flags*/){
            auto* instance = reinterpret_cast<Udp*>(req->data);

            if (nread < 0)
            {
                utils::log::error("Read error: {}", uv_err_name(nread));
                free(buf->base);
                return;
            }

            if (nread == 0)
            {
                instance->m_recvCb("");
                return;
            }

            instance->m_recvCb(std::string(buf->base, nread));
            free(buf->base);
        }));
    }

    void recvStop()
    {
        checkRc(uv_udp_recv_stop(get()));
    }

    void send(const Address& addr, const std::string& message, std::function<void(int32_t)> cb)
    {
        auto buf = uv_buf_init(const_cast<char*>(&message[0]), static_cast<int32_t>(message.size()));

        auto req = std::make_unique<uv_udp_send_t>();
        req->data = new std::function<void(int32_t)>(cb);
        checkRc(uv_udp_send(req.release(), get(), &buf, 1, reinterpret_cast<const sockaddr*>(addr.get()), [] (uv_udp_send_t* req, int status) {
            std::unique_ptr<uv_udp_send_t> reqInst(reinterpret_cast<uv_udp_send_t*>(req));
            std::unique_ptr<std::function<void(int32_t)>> cb(reinterpret_cast<std::function<void(int32_t)>*>(reqInst->data));
            assert(cb);
            (*cb)(status);
        }));
    }

private:
    std::function<void(std::string)>    m_recvCb;
};

class Tcp : public Handle<uv_tcp_t>
{
public:
    Tcp(Loop& loop)
    {
        init(loop, uv_tcp_init);
    }

    void setNoDelay(bool enabled)
    {
        checkRc(uv_tcp_nodelay(get(), enabled ? 1 : 0));
    }

    void setKeepAlive(bool enabled, int32_t delay)
    {
        checkRc(uv_tcp_keepalive(get(), enabled ? 1 : 0, delay));
    }

    void connect(const Address& addr, std::function<void(int32_t)> cb)
    {
        auto conn = std::make_unique<uv_connect_t>();
        conn->data = new std::function<void(int32_t)>(cb);

        checkRc(uv_tcp_connect(conn.release(), get(), reinterpret_cast<const sockaddr*>(addr.get()), [] (uv_connect_t* req, int status) {
            std::unique_ptr<uv_connect_t> conn(req);
            std::unique_ptr<std::function<void(int32_t)>> cb(reinterpret_cast<std::function<void(int32_t)>*>(req->data));
            (*cb)(status);
        }));
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
            handleInstance->close([] () {});
        }
    }, nullptr);
}

struct InterfaceInfo
{
    std::string ipName() const
    {
        std::array<char, 512> name;
        if (isIpv4())
        {
            checkRc(uv_ip4_name(&address.address4, name.data(), name.size()));
        }
        else if (isIpv6())
        {
            checkRc(uv_ip6_name(&address.address6, name.data(), name.size()));
        }
        else
        {
            throw std::runtime_error("Invalid address family");
        }

        return name.data();
    }

    bool isIpv4() const noexcept
    {
        return address.address4.sin_family == AF_INET;
    }

    bool isIpv6() const noexcept
    {
        return address.address4.sin_family == AF_INET6;
    }

    std::string         name;
    std::array<char, 6> physicalAddress;
    bool                isInternal;

    union
    {
        struct sockaddr_in address4;
        struct sockaddr_in6 address6;
    } address;

    union
    {
        struct sockaddr_in netmask4;
        struct sockaddr_in6 netmask6;
    } netmask;
};

inline sockaddr_in createIp4Address(const std::string& ip, int32_t port)
{
    sockaddr_in addr;
    checkRc(uv_ip4_addr(ip.c_str(), port, &addr));
    return addr;
}

inline sockaddr_in6 createIp6Address(const std::string& ip, int32_t port)
{
    sockaddr_in6 addr;
    checkRc(uv_ip6_addr(ip.c_str(), port, &addr));
    return addr;
}

inline std::vector<InterfaceInfo> getInterfaces()
{
    std::vector<InterfaceInfo> res;
    uv_interface_address_s* addresses = nullptr;
    int count = 0;

    checkRc(uv_interface_addresses(&addresses, &count));

    for (int i = 0; i < count; ++i)
    {
        InterfaceInfo addr;
        addr.name = addresses[i].name;
        addr.isInternal = addresses[i].is_internal == 1;

        std::memcpy(addr.physicalAddress.data(), addresses[i].phys_addr, addr.physicalAddress.size());
        std::memcpy(&addr.address, &addresses[i].address, sizeof(addresses[i].address));
        std::memcpy(&addr.netmask, &addresses[i].netmask, sizeof(addresses[i].netmask));

        res.emplace_back(std::move(addr));
    }

    uv_free_interface_addresses(addresses, count);

    return res;
}

}
