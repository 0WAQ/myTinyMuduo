#pragma once

#include "Socket.hpp"
#include "InetAddress.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"

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


    ~Acceptor();

private:
    Socket* _M_serv_sock_ptr;
    Channel* _M_acceptor_channel_ptr;
    EventLoop* _M_loop_ptr;
};