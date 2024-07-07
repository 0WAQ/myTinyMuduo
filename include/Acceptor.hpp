#pragma once

#include "Socket.hpp"
#include "InetAddress.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Connection.hpp"

/**
 * 
 * Channel之上的封装类, 专门用于创建服务端的监听Socket
 *  
 */
class Acceptor
{
public:

    /**
     * 
     * @describe: 初始化loop与服务端监听地址
     * @param:    EventLoop*, const std::string&, const uint16_t
     * 
     */
    Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port);


    /**
     * 
     * @describe: 设置创建连接回调函数的函数, 有TcpServer在创建Acceptor对象时调用
     * @param:    std::function<void(Socket*)>
     * @return:   void
     * 
     */
    void set_create_connection_callback(std::function<void(Socket*)> func);


    /**
     * 
     * @describe: 封装处理新的连接请求的代码
     * @param:    void
     * @return:   void
     * 
     */
    void new_connection();


    ~Acceptor();

private:
    Socket _M_serv_sock;
    Channel* _M_acceptor_channel_ptr;
    EventLoop* _M_loop_ptr;

    // 创建Connection对象的回调函数, 将回调TcpServer::create_connection
    std::function<void(Socket*)> create_connection_callback;
};