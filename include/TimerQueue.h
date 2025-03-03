#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <vector>
#include <memory>
#include <atomic>
#include <set>
#include <map>

#include "TimeStamp.h"
#include "Timer.h"
#include "TimerId.h"
#include "Channel.h"
#include "callbacks.h"
#include "noncopyable.h"

namespace mymuduo
{

class EventLoop;

class TimerQueue : noncopyable
{
public:

    using TimerPtr = std::unique_ptr<Timer>;

public:

    explicit TimerQueue(EventLoop *loop);
    
    ~TimerQueue();

    /**
     * @brief 创建一个timer并添加到定时器队列中
     */
    TimerId add_timer(TimeStamp when, double interval, TimerCallback func);

    /**
     * @brief 取消一个定时器
     */
    void cancel(TimerId timerId);

private:

    using TimerMap = std::multimap<TimeStamp, std::unique_ptr<Timer>>;
    using TimerVec = std::vector<TimerPtr>;
    using ActiveList = std::set<TimerId>;
    using CancelList = std::set<TimerId>;

private:

    void add_timer_in_loop(Timer *timer);

    void cancel_in_loop(TimerId timerId);

    /**
     * @brief 
     */
    void handle_read();

    /**
     * @brief 获取到期的所有定时器
     */
    TimerVec get_expired(TimeStamp now);

    /**
     * @brief 重置超时定时器(若重复则insert, 否则删除), 设置timerfd下次唤醒线程时间
     */
    void reset(TimerVec &expired, TimeStamp now);

    /**
     * @brief 将timer添加到TimerMap和ActiveList中
     */
    bool insert(TimerPtr timer);

private:

    // 定时队列所属的事件循环
    EventLoop *_M_loop;

    const int _M_timer_fd;
    Channel _M_timer_channel;

    // 按到期时间排好序的定时器队列
    TimerMap _M_timers;

    // 保存目前有效的Timer指针, 与TimerMap具有相同的数据
    ActiveList _M_active_timers;

    // 当要删除的定时器正处于expired中时, 那么由于无法将它直接删除, 所以先将其放到该对象中
    CancelList _M_cancel_timers;
    
    // 用于标识线程是否正在处理超时事件的回调函数
    // 在cancel一个定时器时, 若该定时器在expired中(即线程正在执行超时任务), 那么无法将其直接删除, 需加入到CancelList中
    std::atomic<bool> _M_calling_expired_timers;
};

} // namespace mymuduo

#endif // TIMERQUEUE_H
