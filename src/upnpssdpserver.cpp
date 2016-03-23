#include "upnp/upnpssdpserver.h"

#include "utils/log.h"

namespace upnp
{
namespace ssdp
{

void Server::run()
{
    m_socket.read([] (uv_udp_t* /*req*/, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* /*addr*/, unsigned /*flags*/) {
        if (nread < 0)
        {
            utils::log::error("Read error: {}", uv_err_name(nread));
            free(buf->base);
            return;
        }

        utils::log::info("Read data: {}", nread);
    });
}

}
}