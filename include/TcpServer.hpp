#pragma once

#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"

/**
 *  
 * 封装监听Socket的创建和事件循环
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
    EventLoop _M_loop;         // 事件循环变量, 用start方法开始
    Acceptor* _M_acceptor_ptr; // 用于创建监听sock
};