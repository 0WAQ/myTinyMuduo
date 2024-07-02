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
    
    
    /**
     * 
     * @describe: 获取fd
     * @param:    void
     * @return:   int
     * 
     */
    int get_fd();


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
     * @describe: Tcp连接断开后, Channel回调的函数
     * @param:    void
     * @return:   void
     */
    void close_connection();


    /**
     * 
     * @describe: Tcp连接出错后, Channel回调的函数
     * @param:    void
     * @return:   void
     */
    void error_connection();


    ~Connection();

private:
    EventLoop* _M_loop_ptr;
    Socket* _M_clnt_sock_ptr;
    Channel* _M_clnt_channel_ptr;
};