#pragma once
#include "EventLoop.hpp"
#include "Connection.hpp"
#include "TcpServer.hpp"
#include "ThreadPool.hpp"

/**
 * 
 * 回显服务器
 * 支持回显业务
 * 
 */
class EchoServer
{
public:

    /**
     * 
     * @describe: 用于初始化_M_tcp_server
     * @param:    const std::string&, uint16_t, size_t, size_t
     * 
     */
    EchoServer(const std::string& ip, uint16_t port, size_t loop_thread_num = 3, size_t work_thread_um = 5);


    /**
     * 
     * @describe: 发生对应的事件后, TcpServer调用这些函数
     * @param:    void
     * @return:   void
     * 
     */
    void start();
    void handle_deal_message(Connection_ptr conn, std::string& message);
    void handle_create_connection(Connection_ptr conn);
    void handle_close_connection(Connection_ptr conn);
    void handle_error_connection(Connection_ptr conn);
    void handle_send_complete(Connection_ptr conn);
    void handle_epoll_timeout(EventLoop* loop);


    /**
     * 
     * @describe: 业务处理函数
     * @param:    Connection_ptr, std::string&
     * @return:   void
     * 
     */
    void handle_deal_message_a(Connection_ptr conn, std::string& message);


    ~EchoServer();

private:

    // 增加线程池, 用来存放工作线程
    size_t _M_thread_num;
    ThreadPool _M_pool;

    TcpServer _M_tcp_server;
};