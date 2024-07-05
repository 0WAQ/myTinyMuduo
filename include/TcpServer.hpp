#pragma once

#include <map>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"

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


    /**
     * 
     * @describe: 处理客户端报文请求时, Connection回调的函数
     * @param:    Connection*, std::string&
     * @return:   void
     * 
     */
    void deal_message(Connection* conn, std::string& message);


    /**
     * 
     * @describe: 封装处理新的连接请求的代码
     * @param:    Socket*
     * @return:   void
     * 
     */
    void create_connection(Socket* clnt_sock);


    /**
     * 
     * @describe: Tcp连接断开后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void close_connection(Connection* conn);


    /**
     * 
     * @describe: Tcp连接出错后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void error_connection(Connection* conn);


    /**
     * 
     * @describe: 数据发送完成后, 在Connection类中回调此函数
     * @param:    void
     * @return:   void
     * 
     */
    void send_complete(Connection* conn);


    /**
     * 
     * @describe: epoll_wait超时后, 在EventLoop中回调此函数
     * @param:    EventLoop*
     * @return:   void
     * 
     */
    void epoll_timeout(EventLoop* loop);


    // 以下为设置回调函数的函数
    void set_deal_message_callback(std::function<void(Connection*,std::string&)> func);
    void set_create_connection_callback(std::function<void(Connection*)> func);
    void set_close_connection_callback(std::function<void(Connection*)> func);
    void set_error_connection_callback(std::function<void(Connection*)> func);
    void set_send_complete_callback(std::function<void(Connection*)> func);
    void set_epoll_timeout_callback(std::function<void(EventLoop*)> func);


    ~TcpServer();

private:
    // 一个TcpServer中可以有多个事件循环, 在多线程中体现
    EventLoop* _M_main_loop;         // 事件循环变量, 用start方法开始
    Acceptor* _M_acceptor_ptr; // 用于创建监听sock
    std::map<int, Connection*> _M_connections_map;


     // 回调EchoServer::handle_deal_message
    std::function<void(Connection*,std::string&)> _M_deal_message_callback;

    // 回调EchoServer::handle_create_connection
    std::function<void(Connection*)> _M_create_connection_callback;

    // 回调EchoServer::HandleClose
    std::function<void(Connection*)> _M_close_connection_callback;

    // 回调EchoServer::HandleError
    std::function<void(Connection*)> _M_error_connection_callback;

    // 回调EchoServer::HandleSendComplete
    std::function<void(Connection*)> _M_send_complete_callback;

    // 回调EchoServer::HandleTimeOut
    std::function<void(EventLoop*)>  _M_epoll_timeout_callback;
    
};