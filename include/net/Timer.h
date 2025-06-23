#ifndef TIMER_H
#define TIMER_H

#include <atomic>

#include "base/noncopyable.h"
#include "base/TimeStamp.h"
#include "net/callbacks.h"
#include "net/TimerId.h"

namespace mymuduo {
namespace net {

class Timer : noncopyable
{
public:

    Timer(TimeStamp when, TimeDuration interval, TimerCallback cb);

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
    const TimeDuration _M_interval;

    // 是否重复
    bool _M_repeat;

    // 定时器回调函数
    const TimerCallback _M_func;

    // 定时器唯一id
    const TimerId _M_id;

    static std::atomic<ssize_t> _M_last_timerId;
};

} // namespace net
} // namespace mymuduo

#endif // TIMER_H