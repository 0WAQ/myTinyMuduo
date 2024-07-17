#pragma once

#include <map>
#include <mutex>
#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "Connection.hpp"
#include "ThreadPool.hpp"


/// @brief Tcp服务类
class TcpServer
{

    using CreateConnCallback = std::function<void(SpConnection)>;
    using DealMsgCallback = std::function<void(SpConnection, std::string&)>;
    using SendCompleteCallback = std::function<void(SpConnection)>;
    using TimeroutCallback = std::function<void(SpConnection)>;
    using CloseCallback = std::function<void(SpConnection)>;
    using EpollTimeoutCallback = std::function<void(EventLoop*)>;
    using ErrorCallback = std::function<void(SpConnection)>;

public:

    /// @brief 初始化Tcp服务器
    /// @param ip 绑定地址
    /// @param port 绑定ip
    /// @param thread_nums IO线程数
    TcpServer(const std::string& ip, const uint16_t port, size_t thread_nums);


    /// @brief 启动与停止Tcp服务器
    void start();
    void stop();


    /// @brief 各种被调函数
    void create_connection(std::unique_ptr<Socket> clnt_sock);
    void deal_message(SpConnection conn, std::string& message);
    void send_complete(SpConnection conn);
    void timer_out(SpConnection conn);
    void close_connection(SpConnection conn);
    void epoll_timeout(EventLoop* loop);
    void error_connection(SpConnection conn);


    // 以下为设置回调函数的函数
    void set_create_connection_callback(CreateConnCallback func);
    void set_deal_message_callback(DealMsgCallback func);
    void set_send_complete_callback(SendCompleteCallback func);
    void set_timer_out_callback(TimeroutCallback func);
    void set_close_connection_callback(CloseCallback func);
    void set_epoll_timeout_callback(EpollTimeoutCallback func);
    void set_error_connection_callback(ErrorCallback func);

private:

    // 交换顺序, 和初始化列表顺序一致
    size_t _M_thread_num;   // 线程池的大小, 即从事件循环的个数
    ThreadPool _M_pool; // 线程池

    // 一个TcpServer中可以有多个事件循环, 在多线程中体现
    std::unique_ptr<EventLoop> _M_main_loop;         // 事件循环变量, 用start方法开始
    std::vector<std::unique_ptr<EventLoop>> _M_sub_loops; // 从事件, 运行在线程池中

    Acceptor _M_acceptor; // 用于创建监听sock
    std::map<int, SpConnection> _M_connections_map;

    std::mutex _M_mutex; // 用于对map容器的操作上锁

    // 回调EchoServer::handle_create_connection
    CreateConnCallback _M_create_connection_callback;

    // 回调EchoServer::handle_deal_message
    DealMsgCallback _M_deal_message_callback;

    // 回调EchoServer::HandleSendComplete
    SendCompleteCallback _M_send_complete_callback;

    // 回调EchoServer::HandleTimerOut
    TimeroutCallback _M_timer_out_callback;

    // 回调EchoServer::HandleClose
    CloseCallback _M_close_connection_callback;

    // 回调EchoServer::HandleTimeOut
    EpollTimeoutCallback  _M_epoll_timeout_callback;

    // 回调EchoServer::HandleError
    ErrorCallback _M_error_connection_callback;
};