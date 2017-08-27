#include "utils/log.h"
#include "upnp/upnp.action.h"
#include "upnp/upnp.http.functions.h"
#include "upnp.http.client.h"
#include "upnp/upnp.http.server.h"

using namespace upnp;
using namespace asio;
using namespace utils;
using namespace std::string_literals;
using namespace std::chrono_literals;

static const std::string s_hostedFile = "Very small file";

int main(int, char**)
{
    io_service io;
    io_service serverIo;
    http::Server server(io);
    http::Client client(io);

    server.addFile("/test.txt", "plain/text", s_hostedFile);
    server.start(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 0));
    std::thread ioThread([&] () {
        serverIo.run();
    });



    auto task = http::getContentLength(io, server.getWebRootUrl() + "/test.txt");
    task.setCompletionHandler([&] () {
        server.stop();
    });
    std::cout << "Run io" << std::endl;
    io.run();
    sync_await(task);

    ioThread.join();
}
