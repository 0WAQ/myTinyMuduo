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
 *  封装服务端的监听sock
 */
class Socket
{
public:

    /**
     * 
     * @describe: 初始化_M_fd
     * @param:    int
     */
    Socket(int fd);


    /**
     * 
     * @describe: 获取fd
     * @param:    void
     * @return:   int
     * 
     */
    int get_fd() const;


    /**
     * 
     * @describe: 获取ip
     * @param:    void
     * @return:   uint16_t
     */
    std::string get_ip() const;


    /**
     * 
     * @describe: 获取port
     * @param:    void
     * @return:   uint16_t
     * 
     */
    uint16_t get_port() const;


    /**
     * 
     * @describe: 用于监听Socket accept客户连接后, 设置客户的ip和port
     * @param:    const std::string&, uint16_t
     * @return:   void
     * 
     */
    void set_ip_port(const std::string& ip, uint16_t port);


    /**
     * 
     * @describe: 将fd与给定InetAddress绑定
     * @param:    const InetAddress&
     * @return:   void
     * 
     */
    void bind(const InetAddress& serv_addr);


    /**
     * 
     * @describe: 监听该Socket
     * @param:    size_t: 最大连接数
     * @return:   void
     * 
     */
    void listen(size_t max_connection = 128);


    /**
     * 
     * @describe: 接收连接
     * @param:    InetAddress&: 返回的客户端InetAddress
     * @return:   int: clnt_fd
     * 
     */
    int accept(InetAddress& clnt_addr);


    /**
     *  
     * @describe: 用来控制该sockfd的各种属性
     * @param:    bool
     * @return:   void
     * 
     */
    void set_reuse_addr(int opt);   // 地址重用
    void set_reuse_port(int opt);   // 端口重用
    void set_tcp_nodelay(int opt);  // tcp无延迟
    void set_keep_alive(int opt);   // 保持连接

    ~Socket();

private:
    const int _M_fd;
    std::string _M_ip;
    uint16_t _M_port;
};