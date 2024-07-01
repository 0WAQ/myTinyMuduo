#pragma once

#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"

/**
 *  
 * 封装了服务器中创建Socket,绑定InetAddress的过程
 * 并且内含一个EventLoop成员变量, 封装了用Channel绑定servSocket和 EventLoop的过程
 * 
 */
class TcpServer
{
public:

    /**
     * 
     * @describe: 构造函数, 用来初始化Socket
     * @param:    要绑定的ip和端口
     * 
     */
    TcpServer(const std::string& ip, const uint16_t port);


    /**
     * 
     * @describe: 转调用了成员变量_M_loop的run方法
     * @param:    void
     * @return:   void
     * 
     */
    void start();


    ~TcpServer();

private:
    EventLoop _M_loop;
};