#include "InetAddress.h"
#include "Socket.h"
#include "Logger.h"

// bind, listen, accept
void Socket::bind(const InetAddress& serv_addr)
{
    if(::bind(_M_fd, serv_addr.get_addr(), sizeof(sockaddr)) < 0) {
        LOG_ERROR("%s:%s:%d bind error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        close(_M_fd);
    }
}

void Socket::listen(size_t max_connection)
{
    if(::listen(_M_fd, max_connection) != 0) {
        LOG_ERROR("%s:%s:%d listen_fd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        close(_M_fd);     
    }
}

int Socket::accept(InetAddress& clnt_addr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);
    bzero(&addr, sizeof(sockaddr_in));

    int clnt_fd = ::accept4(_M_fd, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    
    if(clnt_fd < 0) {
        LOG_ERROR("%s:%s:%d accept error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        return -1;
    }

    clnt_addr.set_addr(addr);
    
    return clnt_fd;
}

void Socket::shutdown_write() {
    if(::shutdown(_M_fd, SHUT_WR) < 0) {
        LOG_ERROR("%s:%s:%d shutdown write error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

void Socket::set_keep_alive(bool on) {
    ::setsockopt(_M_fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
}

void Socket::set_reuse_addr(bool on) {
    ::setsockopt(_M_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

void Socket::set_reuse_port(bool on) {
    ::setsockopt(_M_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
}

void Socket::set_tcp_nodelay(bool on) {
    ::setsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
}
