#pragma once
#include <unistd.h>
#include <syscall.h>
#include <memory>
#include <functional>
#include <queue>
#include <future>
#include <map>
#include <mutex>
#include <atomic>
#include <sys/eventfd.h> // 利用eventfd唤醒线程
#include <sys/timerfd.h> // 定时器
#include "Epoll.hpp"
#include "Connection.hpp"
#include "TimeStamp.hpp"

class Channel;
class Epoll;
class Connection;
using SpConnection = std::shared_ptr<Connection>;

/**
 *  事件循环类
 *  封装了事件循环的过程, 内含一个Epoll对象
 */
class EventLoop
{
public:

    using EpollTimeoutCallback = std::function<void(EventLoop*)>;
    using TimeroutCallback = std::function<void(SpConnection)>;
    using WorkThreadCallback = std::function<void()>;

public:


    /// @brief 初始化对象
    /// @param main_loop 主事件循环-true, 从事件循环-false
    /// @param timeval   服务器检测定时器事件的周期
    /// @param timeout   定时器超时时间
    EventLoop(bool main_loop, time_t timeval = 30, time_t timeout = 80);


    /// @brief 运行与停止事件循环
    void loop();
    void stop();


    /// @brief 判断当前线程是否为事件循环线程
    /// @return ture-是, false-不是
    bool is_loop_thread();


    /// @brief 用来存放工作线程的任务
    /// @param task 任务
    void push(WorkThreadCallback task);


    /// @brief 唤醒事件循环
    void wakeup();


    /// @brief 唤醒线程后执行的函数
    void handle_eventfd();


    /// @brief 用于处理定时器发生后的待调函数
    void handle_timerfd();


    /// @brief 转调用Epoll中的updata_channel
    /// @param ch
    void updata_channel(Channel* ch);


    /// @brief 转调用Epoll中的remove_channel
    /// @param ch
    void remove_channel(Channel* ch);


    /// @brief 将Connection对象加入到map容器
    /// @param conn 
    void insert(SpConnection conn);


    /// @brief 设置回调函数
    /// @param func 函数对象
    void set_epoll_timeout_callback(EpollTimeoutCallback func);
    void set_timer_out_callback(TimeroutCallback func);

private:

    pid_t _M_tid;
    std::unique_ptr<Epoll> _M_ep_ptr;

    std::vector<Channel*> _M_channels;

    // IO线程的任务队列, 用来存放工作线程的任务
    std::queue<std::function<void()>> _M_task_queue;
    std::mutex _M_mutex;  // 任务同步队列的互斥锁

    int _M_efd; // 用于唤醒事件循环的eventfd
    std::unique_ptr<Channel> _M_ech; // 用于将eventfd加入到epoll

    // 定时器的时间间隔和超时时间
    time_t _M_timeval, _M_timeout;

    int _M_tfd; // 用于清理空闲Connection的timerfd
    std::unique_ptr<Channel> _M_tch; // 用于将timerfd加入到epoll

    bool _M_is_main_loop; // 用于判断当前线程为主线程还是从线程

    std::mutex _M_mmutex; // 用于对map容器的操作上锁

    std::atomic_bool _M_stop = false; // 停止事件循环, true为停止

    // 存放运行在该事件循环上的所有Connection对象
    std::map<int, SpConnection> _M_conns;

    // 当epoll_wait()超时时, 回调TcpServer::epoll_timeout()
    EpollTimeoutCallback _M_epoll_wait_timeout_callback;

    // 当定时器超时时, 回调TcpServer::remove_conn()
    TimeroutCallback _M_timer_out_callback;
};