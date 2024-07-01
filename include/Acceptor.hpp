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
     * @describe: 
     * @param:
     * 
     */
    Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port);


    ~Acceptor();

private:
    Socket* _M_serv_sock_ptr;
    Channel* _M_acceptor_channel_ptr;
    EventLoop* _M_loop_ptr;
};