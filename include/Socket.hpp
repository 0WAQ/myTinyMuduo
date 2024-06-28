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
 *  package the socket's op and fd
 */
class Socket
{
public:
    Socket(int fd);

    /**
     * 
     *  @return: _M_fd
     * 
     */
    int get_fd();

    /**
     * 
     *  package the bind(), listen(), accept4() with the error msg
     * 
     */
    void bind(const InetAddress& serv_addr);
    void listen(size_t max_connection = 128);
    int accept(InetAddress& clnt_addr);

    /**
     * 
     *  @pragma: on: enable the options whether or not 
     * 
     */
    void set_reuse_addr(bool on);
    void set_reuse_port(bool on);
    void set_tcp_nodelay(bool on);
    void set_keep_alive(bool on);

    ~Socket();

private:
    const int _M_fd;
};