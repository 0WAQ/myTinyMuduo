#ifndef MYMUDUO_NET_TIMERQUEUE_H
#define MYMUDUO_NET_TIMERQUEUE_H

#include <vector>
#include <memory>
#include <atomic>
#include <set>
#include <map>

#include "mymuduo/base/Timestamp.h"
#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/Timer.h"
#include "mymuduo/net/TimerId.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/callbacks.h"

namespace mymuduo {
namespace net {

class EventLoop;

class TimerQueue : noncopyable {
public:
    using TimerPtr = std::unique_ptr<Timer>;

public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    TimerId add_timer(Timestamp when, TimeDuration interval, TimerCallback func);
    void cancel(TimerId timerId);

private:
    using TimerMap = std::multimap<Timestamp, std::unique_ptr<Timer>>;
    using TimerVec = std::vector<TimerPtr>;
    using ActiveList = std::set<TimerId>;
    using CancelList = std::set<TimerId>;

private:
    void add_timer_in_loop(Timer *timer);
    void cancel_in_loop(TimerId timerId);

    void handle_read();

    TimerVec get_expired(Timestamp now);

    /**
     * @brief 重置超时定时器(若重复则insert, 否则删除), 设置timerfd下次唤醒线程时间
     */
    void reset(TimerVec &expired, Timestamp now);

    /**
     * @brief 将timer添加到TimerMap和ActiveList中
     */
    bool insert(TimerPtr timer);

private:
    // 定时队列所属的事件循环
    EventLoop *_loop;

    const int _timer_fd;
    Channel _timer_channel;

    // 按到期时间排好序的定时器队列
    TimerMap _timers;

    // 保存目前有效的Timer指针, 与TimerMap具有相同的数据
    ActiveList _active_timers;

    // 当要删除的定时器正处于expired中时, 那么由于无法将它直接删除, 所以先将其放到该对象中
    CancelList _cancel_timers;
    
    // 用于标识线程是否正在处理超时事件的回调函数
    // 在cancel一个定时器时, 若该定时器在expired中(即线程正在执行超时任务), 那么无法将其直接删除, 需加入到CancelList中
    std::atomic<bool> _calling_expired_timers;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERQUEUE_H
