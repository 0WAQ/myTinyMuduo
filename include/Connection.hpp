#pragma once
#include <functional>
#include "Socket.hpp"
#include "EventLoop.hpp"
#include "Channel.hpp"

/**
 *  Channel之上的封装类, 专门用于创建客户端的Socket
 */
class Connection
{
public:

    /**
     * 
     * @describe: 初始化loop与clnt_sock
     * @param:    EventLoop*, Socket*
     * 
     */
    Connection(EventLoop* loop, Socket* clnt_sock);
    
    
    ~Connection();

private:
    EventLoop* _M_loop_ptr;
    Socket* _M_clnt_sock_ptr;
    Channel* _M_clnt_channel_ptr;
};