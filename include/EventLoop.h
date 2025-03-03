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

#include "Poller.h"
#include "TcpConnection.h"
#include "TimerQueue.h"
#include "CurrentThread.h"
#include "callbacks.h"
#include "noncopyable.h"

namespace mymuduo
{

class TcpConnection;

/**
 *  事件循环类
 *  封装了事件循环的过程, 内含一个Epoll对象
 *  主要包含了两个模块 Channel 和 Poller(epoll的抽象)
 */
class EventLoop : noncopyable
{
public:
    
    using ChannelList = std::vector<Channel*>;
    using Functor = std::function<void()>;

public:

    EventLoop();

    ~EventLoop();

    /**
     * @brief 开启与退出事件循环
     */
    void loop();
    void quit();

    /**
     * @brief 判断当前线程是否为subLoops
     */
    bool is_loop_thread() { return _M_tid == CurrentThread::tid(); }

    /**
     * 无论是run_in_loop还是queue_in_loop都要保证任务在IO线程中执行
     * 要保证I/O相关操作在EventLoopThread内完成(线程安全与一致性)
     *  1. 避免数据竞争: I/O操作通常涉及共享资源(fd, buffer, connection)
     *  2. 保证事件处理的顺序: Reactor核心机制是事件驱动, 若其它线程直接操作I/O, 可能会破坏事件处理的顺序, 导致逻辑错误
     *  3. 简化并发设计: 可以避免使用锁机制, 减少死锁和性能瓶颈的风险
     */

    /**
     * @brief 直接执行; 若当前线程是EventLoop所属线程, 则会立即执行, 否则调用queue_in_loop
     */
    void run_in_loop(Functor task);

    /**
     * @brief 延迟执行; 将任务加入EventLoop所属线程的任务队列中, 并唤醒线程
     */
    void queue_in_loop(Functor task);

    /**
     * @brief 将subLoop从poll()中唤醒过来
     */
    void wakeup();

    TimerId run_at(TimeStamp time, TimerCallback func);
    TimerId run_after(double delay, TimerCallback func);
    TimerId run_every(double interval, TimerCallback func);
    void cancel(TimerId timerId);

    /**
     * @brief 转调用Poller中的相应函数
     */
    void update_channel(Channel* ch) { _M_poller->update_channel(ch); }
    void remove_channel(Channel* ch) { _M_poller->remove_channel(ch); }
    bool has_channel(Channel* ch) { return _M_poller->has_channel(ch); }

private:

    /**
     * @brief EventLoop线程被唤醒后执行的回调函数
     */
    void handle_read();

    /**
     * @brief 执行任务队列中的任务
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
        
        // 用于唤醒subLoop
        int _M_wakeup_fd; // 用于唤醒事件循环的eventfd
        std::unique_ptr<Channel> _M_wakeup_channel; // 用于将eventfd加入到epoll


        // IO线程的任务队列, 用来其它线程的任务
        std::vector<Functor> _M_task_queue;
        std::mutex _M_queue_mutex;   // 任务队列的互斥锁

    /**
     * Poller
     */

        // 该事件循环包含的Poller分发器
        std::unique_ptr<Poller> _M_poller;
        
        // poll()返回时的时间
        TimeStamp _M_poller_return_time;

    /**
     * TimerQueue
     */
        std::unique_ptr<TimerQueue> _M_timer_queue;

    /**
     * Channel
     */

        ChannelList _M_activeChannels;

};

} // namespace mymuduo

#endif // EVENTLOOP_H