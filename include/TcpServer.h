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

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using DealMsgCallback = std::function<void(TcpConnectionPtr, std::string&)>;
    using EventsCallback = std::function<void(TcpConnectionPtr)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

public:

    /**
     * @brief 初始化Tcp服务器
     * @param main_loop
     * @param serv_addr
     * @param name
     * @param option
     */
    TcpServer(EventLoop *main_loop, InetAddress &serv_addr,
              const std::string &name, Option option = kNoReusePort);

    ~TcpServer();

    /**
     * @brief 启动TcpServer, 不需要暂停, 会自动析构
     */
    void start();

    void set_thread_num(int num_threads) {
        _M_loop_threads->set_thread_num(num_threads);
    }

    /**
     * @brief 以下为设置回调函数的函数
     */
    void set_connection_callback(ConnectionCallback func) {
        _M_connection_callback = std::move(func);
    }

    void set_thread_init_callback(ThreadInitCallback func) {
        _M_thread_init_callback = std::move(func);
    }

    void set_message_callback(MessageCallback func) {
        _M_message_callback = std::move(func);
    }

    void set_write_complete_callback(WriteCompleteCallback func) {
        _M_write_complete_callback = std::move(func);
    }

private:

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

private:

    /**
     * 
     */
        void new_connection(int clntfd, const InetAddress &clnt_addr);

        void remove_connection(const TcpConnection &conn);

        void remove_connection_in_loop(const TcpConnection &conn);

    /**
     * 
     */
        EventLoop *_M_main_loop;        // 事件循环变量, 用start方法开始
        std::unique_ptr<Acceptor> _M_acceptor; // 用于创建监听sock
        std::shared_ptr<EventLoopThreadPool> _M_loop_threads;

        const std::string _M_ip_port;
        const std::string _M_name;


        ConnectionMap _M_connection_map;
        std::mutex _M_mutex; // 用于对map容器的操作上锁


    /**
     * 
     */
        std::atomic<int> _M_started;
        int _M_next;

    /**
     * @brief 回调函数
     */
        ConnectionCallback _M_connection_callback;
        MessageCallback _M_message_callback;
        WriteCompleteCallback _M_write_complete_callback;
        ThreadInitCallback _M_thread_init_callback;

};

#endif // TCPSERVER_H