#pragma once

#include <map>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"
#include "ThreadPool.hpp"

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
     * @param:    要绑定的ip和端口和线程池的大小
     * 
     */
    TcpServer(const std::string& ip, const uint16_t port, size_t thread_nums);


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
     * @param:    Connection_ptr, std::string&
     * @return:   void
     * 
     */
    void deal_message(Connection_ptr conn, std::string& message);


    /**
     * 
     * @describe: 封装处理新的连接请求的代码
     * @param:    std::unique_ptr<Socket>
     * @return:   void
     * 
     */
    void create_connection(std::unique_ptr<Socket> clnt_sock);


    /**
     * 
     * @describe: Tcp连接断开后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void close_connection(Connection_ptr conn);


    /**
     * 
     * @describe: Tcp连接出错后, Connection回调的函数
     * @param:    void
     * @return:   void
     */
    void error_connection(Connection_ptr conn);


    /**
     * 
     * @describe: 数据发送完成后, 在Connection类中回调此函数
     * @param:    void
     * @return:   void
     * 
     */
    void send_complete(Connection_ptr conn);


    /**
     * 
     * @describe: epoll_wait超时后, 在EventLoop中回调此函数
     * @param:    EventLoop*
     * @return:   void
     * 
     */
    void epoll_timeout(EventLoop* loop);


    // 以下为设置回调函数的函数
    void set_deal_message_callback(std::function<void(Connection_ptr,std::string&)> func);
    void set_create_connection_callback(std::function<void(Connection_ptr)> func);
    void set_close_connection_callback(std::function<void(Connection_ptr)> func);
    void set_error_connection_callback(std::function<void(Connection_ptr)> func);
    void set_send_complete_callback(std::function<void(Connection_ptr)> func);
    void set_epoll_timeout_callback(std::function<void(EventLoop*)> func);


    ~TcpServer();

private:

    ThreadPool* _M_pool_ptr; // 线程池
    size_t _M_thread_num;   // 线程池的大小, 即从事件循环的个数

    // 一个TcpServer中可以有多个事件循环, 在多线程中体现
    EventLoop* _M_main_loop;         // 事件循环变量, 用start方法开始
    std::vector<EventLoop*> _M_sub_loops; // 从事件, 运行在线程池中

    Acceptor* _M_acceptor_ptr; // 用于创建监听sock
    std::map<int, Connection_ptr> _M_connections_map;


     // 回调EchoServer::handle_deal_message
    std::function<void(Connection_ptr, std::string&)> _M_deal_message_callback;

    // 回调EchoServer::handle_create_connection
    std::function<void(Connection_ptr)> _M_create_connection_callback;

    // 回调EchoServer::HandleClose
    std::function<void(Connection_ptr)> _M_close_connection_callback;

    // 回调EchoServer::HandleError
    std::function<void(Connection_ptr)> _M_error_connection_callback;

    // 回调EchoServer::HandleSendComplete
    std::function<void(Connection_ptr)> _M_send_complete_callback;

    // 回调EchoServer::HandleTimeOut
    std::function<void(EventLoop*)>  _M_epoll_timeout_callback;
    
};