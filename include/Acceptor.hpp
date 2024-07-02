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
     * @describe:
     * @param:
     * @return:
     * 
     */
    void set_new_connection_callback(std::function<void(Socket*)> func);


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
    Socket* _M_serv_sock_ptr;
    Channel* _M_acceptor_channel_ptr;
    EventLoop* _M_loop_ptr;
    std::function<void(Socket*)> new_connection_callback;
};