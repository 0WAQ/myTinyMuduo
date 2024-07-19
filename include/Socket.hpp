#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>
#include <cerrno>
#include <unistd.h>

#include "InetAddress.hpp"
#include "Logger.hpp"

int create_non_blocking_fd();

/**
 *  封装socket
 */
class Socket
{
public:

    Socket(int fd);

    /// @brief get fd,ip,port
    /// @return 
    int get_fd() const;
    std::string get_ip() const;
    uint16_t get_port() const;

    void set_ip_port(const std::string& ip, uint16_t port);


    /// @brief 封住三个函数
    void bind(const InetAddress& serv_addr);
    void listen(size_t max_connection = 128);
    int accept(InetAddress& clnt_addr);


    /// @brief socket属性
    /// @param opt 1-启用, 不启用-0
    void set_reuse_addr(int opt);   // 地址重用
    void set_reuse_port(int opt);   // 端口重用
    void set_tcp_nodelay(int opt);  // 不启用naggle算法
    void set_keep_alive(int opt);   // 保持连接

    ~Socket();

private:
    const int _M_fd;
    std::string _M_ip;
    uint16_t _M_port;
};