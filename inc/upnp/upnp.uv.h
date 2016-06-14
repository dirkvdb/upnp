#pragma once

#include <array>
#include <string>
#include <chrono>
#include <future>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <system_error>

#include <experimental/optional>

#include <uv.h>
#include <asio.hpp>

#include "utils/log.h"
#include "upnp/upnp.flags.h"

namespace std
{

template <typename T>
using optional = std::experimental::optional<T>;

}

namespace upnp
{
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

void allocateBuffer(uv_handle_t* /*handle*/, size_t suggestedSize, uv_buf_t* buf)
{
    *buf = uv_buf_init(reinterpret_cast<char*>(std::malloc(suggestedSize)), static_cast<unsigned int>(suggestedSize));
}

}

class Loop;
struct InterfaceInfo;

void stopLoopAndCloseRequests(Loop& loop);
std::vector<InterfaceInfo> getNetworkInterfaces();

class Buffer
{
public:
    enum class Ownership
    {
        Yes,
        No
    };

    Buffer(uint8_t* data, size_t size, Ownership o)
    : m_handle(uv_buf_init(reinterpret_cast<char*>(data), static_cast<unsigned int>(size)))
    , m_ownership(o)
    {
    }

    Buffer(char* data, size_t size, Ownership o)
    : m_handle(uv_buf_init(data, static_cast<unsigned int>(size)))
    , m_ownership(o)
    {
    }

    Buffer(const uv_buf_t* buffer, Ownership o)
    : m_handle(*buffer)
    , m_ownership(o)
    {
    }

    template <typename T>
    Buffer(T& data, Ownership o)
    : m_handle(uv_buf_init(const_cast<char*>(reinterpret_cast<const char*>(data.data())), data.size()))
    , m_ownership(o)
    {
    }

    template <typename T>
    Buffer(const T& data, Ownership o)
    : m_handle(uv_buf_init(const_cast<char*>(reinterpret_cast<const char*>(data.data())), static_cast<unsigned int>(data.size())))
    , m_ownership(o)
    {
        // Client have to remain owner of constant object we cannot free them
        assert(m_ownership == Ownership::No);
    }

    ~Buffer() noexcept
    {
        if (m_ownership == Ownership::Yes)
        {
            free(m_handle.base);
        }
    }

    Buffer(Buffer&&) = default;
    Buffer& operator= (Buffer&&) = default;

    Buffer(const Buffer& other) noexcept
    : m_handle(other.m_handle)
    , m_ownership(other.m_ownership)
    {
        assert(m_ownership == Ownership::No);
    }

    Buffer& operator= (const Buffer& other) noexcept
    {
        assert(other.m_ownership == Ownership::No);
        m_handle = other.m_handle;
        m_ownership = other.m_ownership;
        return *this;
    }

    uv_buf_t* get()
    {
        return &m_handle;
    }

    const uv_buf_t* get() const
    {
        return &m_handle;
    }

    char* data()
    {
        return m_handle.base;
    }

    const char* data() const noexcept
    {
        return m_handle.base;
    }

    size_t size() const noexcept
    {
        return m_handle.len;
    }

private:
    uv_buf_t m_handle;
    Ownership m_ownership;
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

    ~Loop()
    {
        stopLoopAndCloseRequests(*this);

        uv_loop_close(&m_handle);
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

// Wakes up the loop and launches the task
inline void asyncSend(uv::Loop& loop, std::function<void()> cb)
{
    auto handle = std::make_unique<uv_async_t>();
    checkRc(uv_async_init(loop.get(), handle.get(), [] (uv_async_t* asyHandle) {
        std::unique_ptr<uv_async_t> handlePtr(asyHandle);
        std::unique_ptr<std::function<void()>> cbPtr(reinterpret_cast<std::function<void()>*>(handlePtr->data));
        (*cbPtr)();
        uv_close(reinterpret_cast<uv_handle_t*>(handlePtr.release()), [] (uv_handle_t* closeHandle) {
            delete reinterpret_cast<uv_async_t*>(closeHandle);
        });
    }));

    auto cbPtr = std::make_unique<std::function<void()>>(std::move(cb));
    handle->data = cbPtr.get();
    checkRc(uv_async_send(handle.release()));
    cbPtr.release();
}

// Wakes up the loop and launches the task which has a result
template <typename ReturnType>
inline std::future<ReturnType> asyncSend(uv::Loop& loop, std::function<ReturnType()> cb)
{
    struct ContextData
    {
        uv_async_t handle;
        std::function<ReturnType()> cb;
        std::promise<ReturnType> prom;
    };

    auto context = std::make_unique<ContextData>();

    checkRc(uv_async_init(loop.get(), &context->handle, [] (uv_async_t* asyHandle) {
        std::unique_ptr<ContextData> contextPtr(reinterpret_cast<ContextData*>(asyHandle));
        contextPtr->prom.set_value(contextPtr->cb());
        uv_close(reinterpret_cast<uv_handle_t*>(&contextPtr->handle), [] (uv_handle_t* closeHandle) {
            delete reinterpret_cast<ContextData*>(closeHandle);
        });

        contextPtr.release();
    }));

    context->cb = cb;
    auto fut = context->prom.get_future();

    checkRc(uv_async_send(&context->handle));
    context.release();

    return fut;
}

inline void queueWork(uv::Loop& loop, std::function<void()> cb)
{
    auto cbPtr = std::make_unique<std::function<void()>>(std::move(cb));
    auto handle = std::make_unique<uv_work_t>();
    handle->data = cbPtr.get();

    checkRc(uv_queue_work(loop.get(), handle.get(), [] (uv_work_t* asyHandle) {
        auto* cbPtr = reinterpret_cast<std::function<void()>*>(asyHandle->data);
        (*cbPtr)();
    }, [] (uv_work_t* asyHandle, int) {
        delete reinterpret_cast<std::function<void()>*>(asyHandle->data);
        delete asyHandle;
    }));

    handle.release();
    cbPtr.release();
}

// Call this when queueing work from outside the loop thread
inline void queueWorkAsync(uv::Loop& loop, std::function<void()> cb)
{
    asyncSend(loop, [cb, &loop] () {
        queueWork(loop, cb);
    });
}

template <typename HandleType>
class Handle
{
public:
    bool isClosing()
    {
        if (!m_handle)
        {
            return true;
        }

        return uv_is_closing(reinterpret_cast<uv_handle_t*>(get())) != 0;
    }

    // don't call any other member functions after close, we replace the data this ptr with the closed callback
    // and m_handle is released
    void close(std::function<void()> cb)
    {
        if (isClosing())
        {
            return;
        }

        assert(m_handle);

        m_handle->data = new std::function<void()>(std::move(cb));
        uv_close(reinterpret_cast<uv_handle_t*>(m_handle.release()), [] (uv_handle_t* handle) {
            std::unique_ptr<HandleType> handlePtr(reinterpret_cast<HandleType*>(handle));
            std::unique_ptr<std::function<void()>> cbPtr(reinterpret_cast<std::function<void()>*>(handlePtr->data));
            if (cbPtr && *cbPtr)
            {
                (*cbPtr)();
            }
        });
    }

    HandleType* get() noexcept
    {
        assert(m_handle); // if this fires you called a member method after closing
        return m_handle.get();
    }

    const HandleType* get() const noexcept
    {
        assert(m_handle); // if this fires you called a member method after closing
        return m_handle.get();
    }

protected:
    template <typename InitFunc>
    Handle(Loop& loop, InitFunc func)
    : m_loop(loop)
    , m_handle(std::make_unique<HandleType>())
    {
        checkRc(func(m_loop.get(), m_handle.get()));
        m_handle->data = this;
    }

    template <typename InitFunc, typename Arg>
    Handle(Loop& loop, InitFunc func, Arg&& arg)
    : m_loop(loop)
    , m_handle(std::make_unique<HandleType>())
    {
        checkRc(func(m_loop.get(), m_handle.get(), arg));
        m_handle->data = this;
    }

    ~Handle()
    {
        if (!isClosing())
        {
            close(nullptr);
        }
    }

private:
    Loop& m_loop;
    std::unique_ptr<HandleType> m_handle;
};

template <typename HandleType>
class Stream : public Handle<HandleType>
{
public:
    void read(std::function<void(ssize_t, Buffer)> cb)
    {
        m_readCb = std::move(cb);
        checkRc(uv_read_start(reinterpret_cast<uv_stream_t*>(this->get()), allocateBuffer, [] (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
            auto* thisPtr = reinterpret_cast<Stream*>(stream->data);
            thisPtr->m_readCb(nread, Buffer(buf, Buffer::Ownership::Yes));
        }));
    }

    void stopRead()
    {
        checkRc(uv_read_stop(this->get()));
    }

    void write(const Buffer& buf, std::function<void(int32_t)> cb)
    {
        struct Context
        {
            uv_write_t handle;
            std::function<void(int32_t)> cb;
        };

        auto context = std::make_unique<Context>();
        context->cb = std::move(cb);
        checkRc(uv_write(&context->handle, reinterpret_cast<uv_stream_t*>(this->get()), buf.get(), 1, [] (uv_write_t* req, int status) {
            std::unique_ptr<Context> context(reinterpret_cast<Context*>(req));
            if (context->cb)
            {
                context->cb(status);
            }
        }));

        context.release();
    }

    void listen(int32_t backlog, std::function<void(int32_t)> cb)
    {
        m_listenCb = std::move(cb);
        checkRc(uv_listen(reinterpret_cast<uv_stream_t*>(this->get()), backlog, [] (uv_stream_t* req, int status) {
            auto* thisPtr = reinterpret_cast<Stream*>(req->data);
            thisPtr->m_listenCb(status);
        }));
    }

    void accept(Stream& client)
    {
        checkRc(uv_accept(reinterpret_cast<uv_stream_t*>(this->get()), reinterpret_cast<uv_stream_t*>(client.get())));
    }

protected:
    template <typename InitFunc>
    Stream(Loop& loop, InitFunc func)
    : Handle<HandleType>(loop, func)
    {
    }

    template <typename InitFunc, typename Arg>
    Stream(Loop& loop, InitFunc func, Arg&& arg)
    : Handle<HandleType>(loop, func, std::forward<Arg&&>(arg))
    {
    }

private:
    std::function<void(ssize_t, Buffer)> m_readCb;
    std::function<void(int32_t)> m_listenCb;
};

class Idler : public Handle<uv_idle_t>
{
public:
    Idler(Loop& loop, std::function<void()> cb)
    : Handle<uv_idle_t>(loop, uv_idle_init)
    , m_callback(std::move(cb))
    {
    }

    void start()
    {
        checkRc(uv_idle_start(get(), [] (uv_idle_t* handle) {
            reinterpret_cast<Idler*>(handle->data)->m_callback();
        }));
    }

    void stop()
    {
        checkRc(uv_idle_stop(get()));
    }

private:
    std::function<void()> m_callback;
};

template <typename ...Ts>
class Async : public Handle<uv_async_t>
{
public:
    Async(Loop& loop, std::function<void(Ts...)> cb)
    : Handle<uv_async_t>(loop, uv_async_init, &Async::onInit)
    , m_callback(std::move(cb))
    {
    }

    template <typename ...T>
    void send(T&&... args)
    {
        m_args = std::make_tuple(std::forward<T&&>(args)...);
        checkRc(uv_async_send(get()));
    }

private:
    static void onInit(uv_async_t* handle)
    {
        auto thisPtr = reinterpret_cast<Async*>(handle->data);
        apply(thisPtr->m_callback, (thisPtr->m_args));
    }

    std::function<void(Ts...)> m_callback;
    std::tuple<Ts...> m_args;
};

template<>
class Async<void> : public Handle<uv_async_t>
{
public:
    Async(Loop& loop, std::function<void()> cb)
    : Handle<uv_async_t>(loop, uv_async_init, &Async::onInit)
    , m_callback(std::move(cb))
    {
    }

    void send()
    {
        checkRc(uv_async_send(get()));
    }

private:
    static void onInit(uv_async_t* handle)
    {
        reinterpret_cast<Async*>(handle->data)->m_callback();
    }

    std::function<void()> m_callback;
};

class Signal : public Handle<uv_signal_t>
{
public:
    Signal(Loop& loop)
    : Handle<uv_signal_t>(loop, uv_signal_init)
    {
    }

    void start(std::function<void()> cb, int32_t signalNumber)
    {
        m_callback = std::move(cb);
        checkRc(uv_signal_start(get(), [] (uv_signal_t* handle, int /*signum*/) {
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
    Writable = UV_WRITABLE,
    //Disconnect = UV_DISCONNECT >=v1.9.0
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
    : Handle<uv_poll_t>(loop, uv_poll_init, fd)
    {
    }

    Poll(Loop& loop, OsSocket sock)
    : Handle<uv_poll_t>(loop, uv_poll_init, sock)
    {
    }

    void start(Flags<PollEvent> pollEvents, std::function<void(int32_t, Flags<PollEvent>)> cb)
    {
        m_callback = std::move(cb);
        checkRc(uv_poll_start(get(), pollEvents, [] (uv_poll_t* handle, int status, int events) {
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

class Timer : public Handle<uv_timer_t>
{
public:
    Timer(Loop& loop)
    : Handle<uv_timer_t>(loop, uv_timer_init)
    {
    }

    void start(std::chrono::milliseconds timeout, std::function<void()> cb)
    {
        start(timeout, std::chrono::milliseconds(0), cb);
    }

    void start(std::chrono::milliseconds timeout, std::chrono::milliseconds repeat, std::function<void()> cb)
    {
        m_callback = std::move(cb);
        checkRc(uv_timer_start(get(), [] (uv_timer_t* handle) {
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

struct InterfaceInfo
{
    std::string ipName() const
    {
        std::array<char, 512> ipName;
        if (isIpv4())
        {
            checkRc(uv_ip4_name(&address.address4, ipName.data(), ipName.size()));
        }
        else if (isIpv6())
        {
            checkRc(uv_ip6_name(&address.address6, ipName.data(), ipName.size()));
        }
        else
        {
            throw std::runtime_error("Invalid address family");
        }

        return ipName.data();
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

class Address
{
public:
    static Address create(const sockaddr_storage& address, int length)
    {
        Address addr;
        memcpy(&addr.m_address, &address, length);
        return addr;
    }

    static Address createIp4(const sockaddr_in& address)
    {
        Address addr;
        addr.m_address.address4 = address;
        return addr;
    }

    static Address createIp4(const std::string& interfaceName)
    {
        for (auto& intf : getNetworkInterfaces())
        {
            if (intf.isIpv4() && intf.name == interfaceName)
            {
                return uv::Address::createIp4(intf.address.address4);
            }
        }

        throw std::runtime_error("Could not find network interface with name: " + interfaceName);
    }

    static Address createIp4FromHost(const std::string& ipPort, int16_t defaultPort)
    {
        auto pos = ipPort.find(':');
        if (pos == std::string::npos || pos == (ipPort.size() - 1))
        {
            return createIp4(ipPort, defaultPort);
        }
        else
        {
            auto port = std::stoul(ipPort.substr(pos + 1));
            if (port > std::numeric_limits<uint16_t>::max())
            {
                throw std::invalid_argument("Invalid host name: " + ipPort);
            }

            return createIp4(ipPort.substr(0, pos), port);
        }
    }

    static Address createIp4(const std::string& ip, uint16_t port)
    {
        Address addr;
        checkRc(uv_ip4_addr(ip.c_str(), port, &addr.m_address.address4));
        return addr;
    }

    static Address createIp6(const std::string& ip, uint16_t port)
    {
        Address addr;
        checkRc(uv_ip6_addr(ip.c_str(), port, &addr.m_address.address6));
        return addr;
    }

    void setPort(uint16_t port)
    {
        if (isIpv4())
        {
            m_address.address4.sin_port = htons(port);
        }
        else
        {
            m_address.address6.sin6_port = htons(port);
        }
    }

    bool isIpv4() const noexcept
    {
        return m_address.address4.sin_family == AF_INET;
    }

    bool isIpv6() const noexcept
    {
        return m_address.address4.sin_family == AF_INET6;
    }

    std::string ip() const
    {
        std::array<char, 512> name;
        if (isIpv4())
        {
            checkRc(uv_ip4_name(&m_address.address4, name.data(), name.size()));
        }
        else if (isIpv6())
        {
            checkRc(uv_ip6_name(&m_address.address6, name.data(), name.size()));
        }
        else
        {
            throw std::runtime_error("Invalid address family");
        }

        return name.data();
    }

    uint16_t port() const
    {
        if (isIpv4())
        {
            return ntohs(m_address.address4.sin_port);
        }
        else if (isIpv6())
        {
            return ntohs(m_address.address6.sin6_port);
        }
        else
        {
            throw std::runtime_error("Invalid address family");
        }
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
    : Handle<uv_udp_t>(loop, uv_udp_init)
    {
    }

    void bind(const Address& addr, Flags<UdpFlag> flags = Flags<UdpFlag>())
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

    void recv(std::function<void(int32_t status, std::string, std::optional<uv::Address>)> cb)
    {
        m_recvCb = std::move(cb);

        checkRc(uv_udp_recv_start(get(), allocateBuffer, [] (uv_udp_t* req, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned /*flags*/){
            auto* instance = reinterpret_cast<Udp*>(req->data);

            std::optional<uv::Address> ad;

            if (nread < 0)
            {
                utils::log::error("Read error: {}", uv_err_name(static_cast<int>(nread)));
                free(buf->base);
                instance->m_recvCb(static_cast<int32_t>(nread), "", ad);
                return;
            }

            if (nread == 0)
            {
                free(buf->base);

                if (addr != nullptr)
                {
                    // empty udp message received
                    ad = uv::Address::createIp4(*reinterpret_cast<const sockaddr_in*>(addr));
                }
                // else notthing to read from socket

                instance->m_recvCb(0, "", ad);
                return;
            }

            ad = uv::Address::createIp4(*reinterpret_cast<const sockaddr_in*>(addr));
            instance->m_recvCb(0, std::string(buf->base, nread), ad);
            free(buf->base);
        }));
    }

    void recvStop()
    {
        checkRc(uv_udp_recv_stop(get()));
    }

    // Important: the message data has to be valid until the callback is called
    void send(const Address& addr, const std::string& message, std::function<void(int32_t)> cb)
    {
        struct ContextData
        {
            uv_udp_send_t handle;
            std::function<void(int32_t)> cb;
        };

        auto context = std::make_unique<ContextData>();
        context->cb = std::move(cb);

        auto buf = uv_buf_init(const_cast<char*>(&message[0]), static_cast<int32_t>(message.size()));
        checkRc(uv_udp_send(&context->handle, get(), &buf, 1, reinterpret_cast<const sockaddr*>(addr.get()), [] (uv_udp_send_t* req, int status) {
            std::unique_ptr<ContextData> contextPtr(reinterpret_cast<ContextData*>(req));
            if (contextPtr->cb)
            {
                contextPtr->cb(status);
            }
        }));

        context.release();
    }

    Address getSocketName() const
    {
        sockaddr_storage addr;
        int namelen = sizeof(sockaddr_storage);
        checkRc(uv_udp_getsockname(get(), reinterpret_cast<sockaddr*>(&addr), &namelen));

        return Address::create(addr, namelen);
    }

private:
    std::function<void(int32_t, std::string, std::optional<uv::Address>)> m_recvCb;
};

enum class TcpFlag : uint32_t
{
    Ipv6Only = UV_TCP_IPV6ONLY
};

class Tcp : public Stream<uv_tcp_t>
{
public:
    Tcp(Loop& loop)
    : Stream<uv_tcp_t>(loop, uv_tcp_init)
    {
    }

    void setNoDelay(bool enabled)
    {
        checkRc(uv_tcp_nodelay(get(), enabled ? 1 : 0));
    }

    void setKeepAlive(bool enabled, int32_t delay)
    {
        checkRc(uv_tcp_keepalive(get(), enabled ? 1 : 0, delay));
    }

    void bind(const Address& addr, Flags<TcpFlag> flags = Flags<TcpFlag>())
    {
        checkRc(uv_tcp_bind(get(), reinterpret_cast<const sockaddr*>(addr.get()), flags));
    }

    void connect(const Address& addr, std::function<void(int32_t)> cb)
    {
        auto connPtr = std::make_unique<uv_connect_t>();
        connPtr->data = new std::function<void(int32_t)>(std::move(cb));

        checkRc(uv_tcp_connect(connPtr.release(), get(), reinterpret_cast<const sockaddr*>(addr.get()), [] (uv_connect_t* req, int status) {
            std::unique_ptr<uv_connect_t> conn(req);
            std::unique_ptr<std::function<void(int32_t)>> cbPtr(reinterpret_cast<std::function<void(int32_t)>*>(req->data));
            (*cbPtr)(status);
        }));
    }

    Address getSocketName() const
    {
        sockaddr_storage addr;
        int namelen = sizeof(sockaddr_storage);
        checkRc(uv_tcp_getsockname(get(), reinterpret_cast<sockaddr*>(&addr), &namelen));

        return Address::create(addr, namelen);
    }
};

}

inline void stopLoopAndCloseRequests(Loop& loop)
{
    loop.stop();
    loop.run(RunMode::Once);

    bool closed = false;
    uv_walk(loop.get(), [] (uv_handle_t* handle, void* arg) {
        if (uv_is_closing(handle) == 0)
        {
            uv_close(handle, nullptr);
            *reinterpret_cast<bool*>(arg) = true;
        }
    }, &closed);

    if (closed)
    {
        loop.run(RunMode::Once);
    }
}

inline sockaddr_in createIp4Address(const std::string& ip, uint16_t port)
{
    sockaddr_in addr;
    checkRc(uv_ip4_addr(ip.c_str(), port, &addr));
    return addr;
}

inline sockaddr_in6 createIp6Address(const std::string& ip, uint16_t port)
{
    sockaddr_in6 addr;
    checkRc(uv_ip6_addr(ip.c_str(), port, &addr));
    return addr;
}

inline std::vector<InterfaceInfo> getNetworkInterfaces()
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

inline const char* getErrorString(int32_t status)
{
    return uv_strerror(status);
}

}
}
