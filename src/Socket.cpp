#include <iostream>
#include "../include/Socket.hpp"

int create_non_blocking_fd()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(listen_fd < 0) {
        std::cerr << "socket() failed\n";
        printf("%s:%s:%d listen socket create error:%d.\n", 
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
        std::cerr << "bind() failed\n";
        close(_M_fd);
        exit(-1);
    }

    _M_ip = serv_addr.get_ip();
    _M_port = serv_addr.get_port();
}

void Socket::listen(size_t max_connection)
{
    if(::listen(_M_fd, max_connection) != 0) {
        std::cerr << "listen() failed\n";
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

// 设置fd属性的函数
void Socket::set_keep_alive(int opt) {setsockopt(_M_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));}
void Socket::set_reuse_addr(int opt) {setsockopt(_M_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));}
void Socket::set_reuse_port(int opt) {setsockopt(_M_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));}
void Socket::set_tcp_nodelay(int opt) {setsockopt(_M_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));}
