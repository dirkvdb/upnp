#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.functions.h"
#include "upnp.http.client.h"
#include "upnp/upnp.http.server.h"
#include "upnp/upnp.coroutine.h"

#include <experimental/coroutine>

using namespace upnp;
using namespace asio;
using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string s_hostedFile = "Very small file";

// int main(int, char**)
// {
//     io_service io;
//     http::Server server(io);
//     http::Client client(io);

//     server.addFile("/test.txt", "plain/text", s_hostedFile);
//     server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999));

//     auto task = http::getContentLength(io, server.getWebRootUrl() + "/test.txt", 10s);


//     // auto coro = [] () -> sync_await_helper {
//     //     std::cout << "Coro ret" << std::endl;
//     //     co_return;
//     // }();

//     // coro.setCompletionHandler([&] () {
//     //     io.stop();
//     // });

//     //task.await_suspend(coro.handle);

//     task.resume();

//     std::tuple<http::StatusCode, size_t> res;
//     std::thread t([&] () {
//         res = sync_await(task);
//     });

//     std::cout << "Run io" << std::endl;
//     io.run();
//     std::cout << "Join t" << std::endl;
//     t.join();

//     //std::tuple<http::StatusCode, size_t> res = task.await_resume();
//     std::cout << "Res " << std::get<1>(res) << std::endl;
//     return 0;
// }

// int main(int, char**)
// {
//     io_service io;
//     http::Server server(io);
//     http::Client client(io);

//     server.addFile("/test.txt", "plain/text", s_hostedFile);
//     server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999));

//     asio::ip::tcp::socket socket(io);
//     auto addr = ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999);
//     socket.open(addr.protocol());
//     auto task = async_connect(socket, addr);

//     auto coro = [] () -> sync_await_helper {
//         std::cout << "Coro ret" << std::endl;
//         co_return;
//     }();

//     coro.setCompletionHandler([&] () {
//         io.stop();
//     });

//     task.await_suspend(coro.handle);

//     std::cout << "Run io" << std::endl;
//     io.run();
//     return 0;
// }

Future<void> asioConnect(asio::ip::tcp::socket& s, const asio::ip::tcp::endpoint& ep)
{
    std::cout << "asioConnect" << std::endl;
    s.open(ep.protocol());
    co_await async_connect(s, ep);
    std::cout << "asioConnect await done" << std::endl;
}

void stopIo(io_service& io)
{
    io.post([&io] () {
        io.stop();
        std::cout << "io stopped" << std::endl;
    });
}

static Future<void> doIt(io_service& io, asio::ip::tcp::socket& socket)
{
    //std::cout << "Await task" << std::endl;
    //co_await task;

    auto ep = ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999);
    std::cout << "asioConnect" << std::endl;
    socket.open(ep.protocol());
    co_await async_connect(socket, ep);
    std::cout << "asioConnect await done" << std::endl;
    //std::cout << "Await task done" << std::endl;

    std::cout << "stop io" << std::endl;
    io.stop();
}

template <typename Awaitable>
Future<void> doIt(io_service& io, Awaitable&& awaitable)
{
    std::cout << "------- Await task" << std::endl;
    auto ret = co_await awaitable;
    std::cout << "#################" << std::endl;
    std::cout << "asioConnect await done: " << std::get<1>(ret) << std::endl;
    std::cout << "#################" << std::endl;
    //std::cout << "Await task done" << std::endl;

    std::cout << "stop io" << std::endl;
    io.stop();
    std::cout << "io stopped" << std::endl;
}

int main(int, char**)
{
    io_service io;
    http::Server server(io);
    http::Client client(io);

    server.addFile("/test.txt", "plain/text", s_hostedFile);
    server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999));

    asio::ip::tcp::socket socket(io);

    Future<void> fut;
//    io.post([&] () {
//        fut = doIt(io, socket);
//    });

    //auto task = asioConnect(socket, ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 9999));
    auto task = http::getContentLength(io, server.getWebRootUrl() + "/test.txt");
    io.post([&] () {
        fut = doIt(io, task);
    });

    std::cout << "Run io" << std::endl;
    io.run();
    std::cout << "Io ready" << std::endl;
    return 0;
}
