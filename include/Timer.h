#ifndef TIMER_H
#define TIMER_H

#include <atomic>

#include "TimeStamp.h"
#include "callbacks.h"
#include "noncopyable.h"

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

    TimeStamp expiration() { return _M_expiration; }
    bool repeat() { return _M_repeat; }
    long id() { return _M_id; }
    long last_timerId() { return _M_last_timerId; }

private:

    // 到期时间
    TimeStamp _M_expiration;

    // 间隔时间
    const double _M_interval;

    // 是否重复
    bool _M_repeat;

    // 定时器回调函数
    const TimerCallback _M_func;

    // 定时器id
    const long _M_id;

    static std::atomic<long> _M_last_timerId;
};

#endif // TIMER_H