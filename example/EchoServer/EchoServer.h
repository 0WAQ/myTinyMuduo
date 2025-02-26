/**
 * 
 * 简易的EchoServer实现
 * 
 */
#ifndef ECHOSERVER_H
#define ECHOSERVER_H

#include "EventLoop.h"
#include "Connection.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Logger.h"


/// @brief 业务服务器: Echo
class EchoServer
{
public:

    /// @brief 初始化Echo
    /// @param loop_thread_num  IO线程数
    /// @param work_thread_um   WORK线程数
    EchoServer(EventLoop *main_loop, InetAddress addr, size_t loop_thread_num = 3, size_t work_thread_um = 5);


    /// @brief 启动与停止
    void start();
    void stop();

    /// @brief 业务处理的被调函数
    void handle_deal_message(SpConnection conn, std::string& message);
    void handle_create_connection(SpConnection conn);
    void handle_close_connection(SpConnection conn);
    void handle_error_connection(SpConnection conn);
    void handle_send_complete(SpConnection conn);
    void handle_timer_out(SpConnection conn);

    /// @brief 中间层, 以便将发送交给IO线程完成
    void handle_deal_message_a(SpConnection conn, std::string& message);


    ~EchoServer();

private:

    // 增加线程池, 用来存放工作线程
    size_t _M_thread_num;
    ThreadPool _M_pool;

    TcpServer _M_tcp_server;

    bool _M_is_stop = false;
};

#endif // ECHOSERVER_H