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
#include "upnp/upnp.flags.h"

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

void allocateBuffer(uv_handle_t* /*handle*/, size_t suggested_size, uv_buf_t* buf)
{
    *buf = uv_buf_init(reinterpret_cast<char*>(std::malloc(suggested_size)), suggested_size);
}

}

class Loop;
void stopLoopAndCloseRequests(Loop& loop);

class Buffer
{
public:
    enum class Ownership
    {
        Yes,
        No
    };

    Buffer(uint8_t* data, size_t size, Ownership o)
    : m_handle(uv_buf_init(reinterpret_cast<char*>(data), size))
    , m_ownership(o)
    {
    }

    Buffer(char* data, size_t size, Ownership o)
    : m_handle(uv_buf_init(data, size))
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
    : m_handle(uv_buf_init(const_cast<char*>(reinterpret_cast<const char*>(data.data())), data.size()))
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

    Buffer(const Buffer&) = delete;
    Buffer& operator= (const Buffer&) = delete;

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

    void close(std::function<void()> cb)
    {
        if (isClosing())
        {
            return;
        }
        
        assert(m_handle);

        m_handle->data = new std::function<void()>(std::move(cb));
        uv_close(reinterpret_cast<uv_handle_t*>(m_handle.release()), [] (auto* handle) {
            std::unique_ptr<HandleType> handlePtr(reinterpret_cast<HandleType*>(handle));
            std::unique_ptr<std::function<void()>> cb(reinterpret_cast<std::function<void()>*>(handlePtr->data));
            if (cb && *cb)
            {
                (*cb)();
            }
        });
    }

    HandleType* get() noexcept
    {
        return m_handle.get();
    }

    const HandleType* get() const noexcept
    {
        return m_handle.get();
    }

protected:
    Handle()
    : m_handle(std::make_unique<HandleType>())
    {
    }
    
    ~Handle()
    {
        if (m_handle && !isClosing())
        {
            close(nullptr);
        }
    }

    template <typename InitFunc>
    void init(Loop& loop, InitFunc func)
    {
        checkRc(func(loop.get(), m_handle.get()));
        m_handle->data = this;
    }

    template <typename InitFunc, typename Arg>
    void init(Loop& loop, InitFunc func, Arg&& arg)
    {
        checkRc(func(loop.get(), m_handle.get(), arg));
        m_handle->data = this;
    }

private:
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
        auto write = std::make_unique<uv_write_t>();
        write->data = new std::function<void(int32_t)>(cb);
        checkRc(uv_write(write.release(), reinterpret_cast<uv_stream_t*>(this->get()), buf.get(), 1, [] (uv_write_t* req, int status) {
            std::unique_ptr<uv_write_t> writePtr(req);
            std::unique_ptr<std::function<void(int32_t)>> cb(reinterpret_cast<std::function<void(int32_t)>*>(writePtr->data));
            (*cb)(status);
        }));
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

private:
    std::function<void(ssize_t, Buffer)> m_readCb;
    std::function<void(int32_t)> m_listenCb;
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
    static Address create(const sockaddr_storage& address, int length)
    {
        Address addr;
        memcpy(&addr.m_address, &address, length);
        return addr;
    }

    static Address createIp4(sockaddr_in address)
    {
        Address addr;
        addr.m_address.address4 = address;
        return addr;
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
    {
        init(loop, uv_udp_init);
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
                free(buf->base);
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

enum class TcpFlag : uint32_t
{
    Ipv6Only = UV_TCP_IPV6ONLY
};

class Tcp : public Stream<uv_tcp_t>
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

    void bind(const Address& addr, Flags<TcpFlag> flags = Flags<TcpFlag>())
    {
        checkRc(uv_tcp_bind(get(), reinterpret_cast<const sockaddr*>(addr.get()), flags));
    }

    void connect(const Address& addr, std::function<void(int32_t)> cb)
    {
        auto conn = std::make_unique<uv_connect_t>();
        conn->data = new std::function<void(int32_t)>(std::move(cb));

        checkRc(uv_tcp_connect(conn.release(), get(), reinterpret_cast<const sockaddr*>(addr.get()), [] (uv_connect_t* req, int status) {
            std::unique_ptr<uv_connect_t> conn(req);
            std::unique_ptr<std::function<void(int32_t)>> cb(reinterpret_cast<std::function<void(int32_t)>*>(req->data));
            (*cb)(status);
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

}
}
