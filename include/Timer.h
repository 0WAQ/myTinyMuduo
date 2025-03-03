#ifndef TIMER_H
#define TIMER_H

#include <atomic>

#include "TimerId.h"
#include "TimeStamp.h"
#include "callbacks.h"
#include "noncopyable.h"

namespace mymuduo
{

class Timer : noncopyable
{
public:

    Timer(TimeStamp when, double interval, TimerCallback cb);

    /**
     * @brief 运行定时任务
     */
    void run() { _M_func(); }
    
    /**
     * @brief 重启定时器
     */
    void restart(TimeStamp now);

    TimeStamp expiration() const { return _M_expiration; }
    bool repeat() const { return _M_repeat; }
    TimerId id() const { return _M_id; }
    ssize_t last_timerId() const { return _M_last_timerId; }

private:

    // 到期时间
    TimeStamp _M_expiration;

    // 间隔时间
    const double _M_interval;

    // 是否重复
    bool _M_repeat;

    // 定时器回调函数
    const TimerCallback _M_func;

    // 定时器唯一id
    const TimerId _M_id;

    static std::atomic<ssize_t> _M_last_timerId;
};

} // namespace mymuduo

#endif // TIMER_H