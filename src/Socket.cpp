#include <iostream>
#include "../include/Socket.h"

// 创建非阻塞的sockfd
int create_non_blocking_fd()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(listen_fd < 0) {
        LOG_ERROR("%s:%s:%d listen_fd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return listen_fd;
}

Socket::Socket(int fd):_M_fd(fd) { }
Socket::~Socket() { close(_M_fd);}

// bind, listen, accept
void Socket::bind(const InetAddress& serv_addr)
{
    if(::bind(_M_fd, serv_addr.get_addr(), sizeof(sockaddr)) < 0) {
        LOG_ERROR("%s:%s:%d bind error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        close(_M_fd);
        exit(-1);
    }

    _M_ip = serv_addr.get_ip();
    _M_port = serv_addr.get_port();
}

void Socket::listen(size_t max_connection)
{
    if(::listen(_M_fd, max_connection) != 0) {
        LOG_ERROR("%s:%s:%d listen_fd create error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        close(_M_fd);
        exit(-1);      
    }
}

int Socket::accept(InetAddress& clnt_addr)
{
    sockaddr_in clnt_addr1;
    socklen_t clnt_addr1_len = sizeof(clnt_addr1);

    int clnt_fd = ::accept4(_M_fd, (sockaddr*)&clnt_addr1, &clnt_addr1_len, SOCK_NONBLOCK);
    clnt_addr.set_addr(clnt_addr1);
    
    if(clnt_fd < 0) {
        LOG_ERROR("%s:%s:%d accept error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
        return -1;
    }
    
    return clnt_fd;
}

// 设置ip, port
void Socket::set_ip_port(const std::string& ip, uint16_t port)
{
    _M_ip = ip;
    _M_port = port;
}

// 获取fd, ip, port
int Socket::         get_fd()   const { return _M_fd;}
std::string Socket:: get_ip()   const { return _M_ip;}
uint16_t Socket::    get_port() const { return _M_port;}

void Socket::shutdown_write() {
    if(::shutdown(_M_fd, SHUT_WR) < 0) {
        LOG_ERROR("%s:%s:%d shutdown write error:%d.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno);
    }
}

// 设置fd属性的函数
void Socket::set_keep_alive(int on) {setsockopt(_M_fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));}
void Socket::set_reuse_addr(int on) {setsockopt(_M_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));}
void Socket::set_reuse_port(int on) {setsockopt(_M_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));}
void Socket::set_tcp_nodelay(int on) {setsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));}
