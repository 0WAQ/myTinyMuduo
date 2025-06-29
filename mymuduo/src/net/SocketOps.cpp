#include "mymuduo/base/Logger.h"
#include "mymuduo/net/SocketOps.h"

#include <netinet/in.h>
#include <strings.h>

namespace mymuduo {
namespace net {
namespace sockets {

void close(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_ERROR("%s:%s:%d close error:%d.\n",
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

sockaddr_in get_local_addr(int sockfd) {
    sockaddr_in local;
    ::bzero(&local, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getsockname(sockfd, (sockaddr*)&local, &len) < 0) {
        LOG_ERROR("InetAddress::get_local_addr.\n");
    }
    return local;
}

sockaddr_in get_peer_addr(int sockfd) {
    sockaddr_in peer;
    ::bzero(&peer, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getpeername(sockfd, (sockaddr*)&peer, &len) < 0) {
        LOG_ERROR("InetAddress::get_peer_addr,\n");
    }
    return peer;
}

bool is_self_connect(int sockfd) {
    struct sockaddr_in localaddr = sockets::get_local_addr(sockfd);
    struct sockaddr_in peeraddr = sockets::get_peer_addr(sockfd);

    if (localaddr.sin_family == AF_INET) {
        const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
        const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port
            && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin_family == AF_INET6) {
        return localaddr.sin_port == peeraddr.sin_port
            && memcmp(&localaddr.sin_addr, &peeraddr.sin_addr, sizeof localaddr.sin_addr) == 0;
    }
    else {
        return false;
    }
}

} // namespace sockets
} // namespace net
} // namespace mymuduo