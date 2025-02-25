/**
 * 
 * EventLoop头文件
 * 
 */
#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <map>
#include <sys/eventfd.h> // 利用eventfd唤醒线程
#include <sys/timerfd.h> // 定时器

#include "Poller.h"
#include "Connection.h"
#include "TimeStamp.h"
#include "CurrentThread.h"
#include "noncopyable.h"

class Connection;

/**
 *  事件循环类
 *  封装了事件循环的过程, 内含一个Epoll对象
 *  主要包含了两个模块 Channel 和 Poller(epoll的抽象)
 */
class EventLoop : noncopyable
{
public:
    
    // TODO:

    using SpConnection = std::shared_ptr<Connection>;
    using ChannelList = std::vector<Channel*>;
    using Functor = std::function<void()>;

    using TimeroutCallback = std::function<void(SpConnection)>;

public:

    /**
     * @brief 初始化对象
     * @param main_loop 主事件循环-true, 从事件循环-false TODO:
     * @param timeval   服务器检测定时器事件的周期
     * @param timeout   定时器超时时间
     */
    EventLoop(bool main_loop, time_t timeval = 30, time_t timeout = 80);

    ~EventLoop();

    /**
     * @brief 开启事件循环
     */
    void loop();

    /**
     * @brief 退出事件循环
     */
    void quit();

    /**
     * @brief 判断当前线程是否为事件循环线程
     */
    inline bool is_loop_thread() { return _M_tid == CurrentThread::tid(); }

    /**
     * 无论是run_in_loop还是queue_in_loop都要保证任务在IO线程中执行
     */

    /**
     * @brief 直接在当前loop中执行任务
     */
    void run_in_loop(Functor task);

    /**
     * @brief 将其放在loop的队列中, 唤醒对应线程去执行
     */
    void queue_in_loop(Functor task);

    /**
     * @brief 将subLoop从pool中唤醒过来
     */
    void wakeup();

    /**
     * @brief 转调用Epoll中的updata_channel
     */
    void update_channel(Channel* ch);

    /**
     * @brief 转调用Epoll中的remove_channel
     */
    void remove_channel(Channel* ch);

    /**
     * @brief 当前loop中是否含有ch
     */
    bool has_channel(Channel* ch);


    // TODO:
    /**
     * @brief 将Connection对象加入到map容器
     */
    void insert(SpConnection conn);

    /**
     * @brief 设置回调函数
     */
    void set_timer_out_callback(TimeroutCallback func);

private:

    /**
     * 内部接口
     */

    /**
     * @brief 唤醒线程后执行的函数
     */
    void handle_read();

    /**
     * @brief 用于处理定时器发生后的待调函数
     */
    void handle_timer();

    /**
     * @brief loop对应的线程被唤醒后, 执行task中的任务
     */
    void do_pending_functors();

private:

    /**
     * EventLoop
     */
    
        std::atomic<bool> _M_looping = false;
        std::atomic<bool> _M_quit    = false;

        // 标志当前loop是否正在执行任务(特指从任务队列中取出的任务)
        std::atomic<bool> _M_calling_pending_functors = false;
        
        // 与EventLoop绑定的线程tid
        const pid_t _M_tid;
        
        // 当
        int _M_wakeup_fd; // 用于唤醒事件循环的eventfd
        std::unique_ptr<Channel> _M_wakeup_channel; // 用于将eventfd加入到epoll


        // IO线程的任务队列, 用来其它线程的任务
        std::vector<Functor> _M_task_queue;
        std::mutex _M_queue_mutex;   // 任务队列的互斥锁

    /**
     * Poller相关
     */

        // 该事件循环包含的Poller分发器
        std::unique_ptr<Poller> _M_poller;
        
        // poll返回时的时间
        TimeStamp _M_poller_return_time;

    /**
     * Channel相关
     */

        ChannelList _M_activeChannels;

    /**
     * 定时器相关 TODO:
     */
        int _M_tfd; // 用于清理空闲Connection的timerfd
        std::unique_ptr<Channel> _M_tch; // 用于将timerfd加入到epoll

        // 定时器的时间间隔和超时时间
        time_t _M_timeval, _M_timeout;

        bool _M_is_main_loop; // 用于判断当前线程为主线程还是从线程

        // 存放运行在该事件循环上的所有Connection对象
        std::map<int, SpConnection> _M_conns;
        std::mutex _M_map_mutex; // 用于对map容器的操作上锁

        // 当定时器超时时, 回调TcpServer::remove_conn()
        TimeroutCallback _M_timer_out_callback;

};

#endif // EVENTLOOP_H