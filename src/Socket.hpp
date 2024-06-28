#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>
#include <cerrno>
#include <unistd.h>

#include "InetAddress.hpp"


int create_non_blocking_fd();

/**
 * 
 */
class Socket
{
public:
    Socket(int fd);

    int get_fd();
    void bind(const InetAddress& serv_addr);
    void listen(size_t max_connection = 128);
    int accept(InetAddress& clnt_addr);
    void set_reuse_addr(bool on);
    void set_reuse_port(bool on);
    void set_tcp_nodelay(bool on);
    void set_keep_alive(bool on);

    ~Socket();
private:
    Socket(const Socket&) = delete;
    const int _M_fd;
};