#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/Socket.h"
#include "mymuduo/net/SocketOps.h"

#include <asm-generic/socket.h>
#include <cerrno>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

using namespace mymuduo;
using namespace mymuduo::net;

Socket::Socket(int fd)
    : _M_fd(fd)
{ }

Socket::~Socket() {
    if (_M_closed) {
        return;
    }
    close();
}

// bind, listen, accept
void Socket::bind(const InetAddress& serv_addr) {
    sockets::bind(_M_fd, serv_addr.addr());
}

void Socket::listen() {
    sockets::listen(_M_fd);
}

int Socket::accept(InetAddress& clnt_addr) {
    return sockets::accept(_M_fd, clnt_addr.addr());
}

void Socket::close() {
    _M_closed = true;
    sockets::close(_M_fd);
}

void Socket::shutdown_write() {
    sockets::shutdown_write(_M_fd);
}

void Socket::set_keep_alive(bool on) {
    sockets::setsockopt(_M_fd, SOL_SOCKET, SO_KEEPALIVE, on);
}

void Socket::set_reuse_addr(bool on) {
    sockets::setsockopt(_M_fd, SOL_SOCKET, SO_REUSEADDR, on);
}

void Socket::set_reuse_port(bool on) {
    sockets::setsockopt(_M_fd, SOL_SOCKET, SO_REUSEPORT, on);
}

void Socket::set_tcp_nodelay(bool on) {
    sockets::setsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, on);
}

