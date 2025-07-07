#ifndef MYMUDUO_NET_TCPSERVER_H
#define MYMUDUO_NET_TCPSERVER_H

#include <condition_variable>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/callbacks.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/EventLoopThread.h"
#include "mymuduo/net/EventLoopThreadPool.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/Acceptor.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/InetAddress.h"

namespace mymuduo {
namespace net {
namespace __detail {
    /**
     * @brief 判断loop非空, 确保用户不会传入空的main_loop
     */
    EventLoop* check_loop_not_null(EventLoop*);

} // namespace __detail

/**
 * @brief 对外的接口类
 */
class TcpServer : noncopyable {
public:
    enum Option {
        kNoReusePort,
        kReusePort,
    };

public:
    TcpServer(EventLoop *main_loop, const InetAddress &serv_addr,
              const std::string &name, Option option = kNoReusePort, bool is_ET = false);

    ~TcpServer();

    /**
     * @brief 启动TcpServer, 不需要暂停, 会自动析构
     */
    void start();
    void stop();

    /**
     * @brief 设置从EventLoop线程的数量, 需在启动前调用
     */
    void set_thread_num(int num_threads) {
        _loop_threads->set_thread_num(num_threads);
    }

    void set_connection_callback(ConnectionCallback func) { _connection_callback = std::move(func); }
    void set_message_callback(MessageCallback func) { _message_callback = std::move(func); }
    void set_write_complete_callback(WriteCompleteCallback func) { _write_complete_callback = std::move(func); }
    void set_thread_init_callback(ThreadInitCallback func) { _thread_init_callback = std::move(func); }

    const InetAddress& listen_addr() const { return _acceptor->listen_addr(); }    

private:
    using ConnectionMap = std::unordered_map<size_t, TcpConnectionPtr>;

    void new_connection(int clntfd, const InetAddress &clnt_addr);
    void remove_connection(const TcpConnectionPtr &conn);
    void remove_connection_in_loop(const TcpConnectionPtr &conn);

private:
    const std::string _name;      // 服务器名称
    const std::string _ip_port;   // 服务器地址信息

    // 主事件循环
    EventLoop *_main_loop;
    std::unique_ptr<Acceptor> _acceptor;

    // 从事件循环
    std::shared_ptr<EventLoopThreadPool> _loop_threads;
    ConnectionMap _connections;

    std::condition_variable _connections_cond;
    std::mutex _connections_mutex;

    size_t _next;    // 连接的编号, 从1开始

    std::atomic<int> _started;
    std::atomic<bool> _stopping;

    bool _is_ET;

    ConnectionCallback _connection_callback;
    MessageCallback _message_callback;
    WriteCompleteCallback _write_complete_callback;
    HighWaterMarkCallback _high_water_mark_callback;
    ThreadInitCallback _thread_init_callback;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TCPSERVER_H