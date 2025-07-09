#include "mymuduo/base/Logger.h"
#include "mymuduo/net/SocketOps.h"

#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

namespace mymuduo {
namespace net {
namespace sockets {

int create_non_blocking_fd() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
    return sockfd;
}

int setsockopt(int sockfd, int level, int optname, bool on) {
    int opt = on ? 1 : 0;
    return ::setsockopt(sockfd, level, optname, &on, sizeof(opt));
}

void bind(int sockfd, struct sockaddr* addr) {
    int ret = ::bind(sockfd, addr, sizeof(sockaddr));
    if(ret < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

void listen(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

int accept(int listenfd, struct sockaddr* addr) {
    socklen_t len = sizeof(sockaddr);
    bzero(addr, sizeof(sockaddr));

    int clnt_fd = ::accept4(listenfd, addr, &len,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);    
    if(clnt_fd < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
    return clnt_fd;
}

int connect(int sockfd, struct sockaddr* addr) {
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr)));
}

void close(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

void shutdown_write(int sockfd) {
    if(::shutdown(sockfd, SHUT_WR) < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
}

int get_socket_error(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}

sockaddr_in get_local_addr(int sockfd) {
    sockaddr_in local;
    ::bzero(&local, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getsockname(sockfd, (sockaddr*)&local, &len) < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    }
    return local;
}

sockaddr_in get_peer_addr(int sockfd) {
    sockaddr_in peer;
    ::bzero(&peer, sizeof(sockaddr_in));
    socklen_t len = sizeof(sockaddr_in);
    if(::getpeername(sockfd, (sockaddr*)&peer, &len) < 0) {
        LOG_ERROR("{}:{}:{} - errno = {} {}.", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
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
    else {
        return false;
    }
}

} // namespace sockets
} // namespace net
} // namespace mymuduo