/**
 * 
 * TcpServer头文件
 * 
 */
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "Logger.h"
#include "ThreadPool.h"
#include "callbacks.h"
#include "noncopyable.h"

/**
 * @brief 对外的接口类
 */
class TcpServer : noncopyable
{
public:

    enum Option {
        kNoReusePort,
        kReusePort,
    };

public:

    /**
     * @brief 初始化Tcp服务器
     * @param main_loop 主事件循环
     * @param serv_addr 服务端地址信息
     * @param name 服务器名称
     * @param option kNoReusePort -- 不复用端口, kReusePort -- 端口复用
     * @param is_ET 是否采用ET模式
     */
    TcpServer(EventLoop *main_loop, const InetAddress &serv_addr,
              const std::string &name, Option option = kNoReusePort, bool is_ET = false);

    ~TcpServer();

    /**
     * @brief 启动TcpServer, 不需要暂停, 会自动析构
     */
    void start();

    /**
     * @brief 设置从EventLoop线程的数量, 需在启动前调用
     */
    void set_thread_num(int num_threads) {
        _M_loop_threads->set_thread_num(num_threads);
    }


    void set_connection_callback(ConnectionCallback func) { _M_connection_callback = std::move(func); }
    void set_message_callback(MessageCallback func) { _M_message_callback = std::move(func); }
    void set_write_complete_callback(WriteCompleteCallback func) { _M_write_complete_callback = std::move(func); }
    void set_thread_init_callback(ThreadInitCallback func) { _M_thread_init_callback = std::move(func); }

private:

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    /**
     * 内部方法
     */

        void new_connection(int clntfd, const InetAddress &clnt_addr);

        void remove_connection(const TcpConnectionPtr &conn);

        void remove_connection_in_loop(const TcpConnectionPtr &conn);

private:

        const std::string _M_name;      // 服务器名称
        const std::string _M_ip_port;   // 服务器地址信息

        // 主事件循环
        EventLoop *_M_main_loop;
        std::unique_ptr<Acceptor> _M_acceptor;
        
        // 从事件循环
        std::shared_ptr<EventLoopThreadPool> _M_loop_threads;
        ConnectionMap _M_connections;
        std::mutex _M_mutex;

        int _M_next;    // 下一个连接所属的EventLoop的索引

        std::atomic<int> _M_started;

        bool _M_is_ET;

    /**
     * @brief 上层设置的回调函数
     */
        ConnectionCallback _M_connection_callback;
        MessageCallback _M_message_callback;
        WriteCompleteCallback _M_write_complete_callback;
        HighWaterMarkCallback _M_high_water_mark_callback;
        ThreadInitCallback _M_thread_init_callback;
};

#endif // TCPSERVER_H