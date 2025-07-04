#ifndef MYMUDUO_NET_TIMER_H
#define MYMUDUO_NET_TIMER_H

#include <atomic>

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/callbacks.h"
#include "mymuduo/net/TimerId.h"

namespace mymuduo {
namespace net {

class Timer : noncopyable {
public:
    Timer(Timestamp when, TimeDuration interval, TimerCallback cb);

    void run() { _M_func(); }
    void restart(Timestamp now);

    Timestamp expiration() const { return _M_expiration; }
    bool repeat() const { return _M_repeat; }
    TimerId id() const { return _M_id; }
    ssize_t last_timerId() const { return _M_last_timerId; }

private:
    // 到期时间
    Timestamp _M_expiration;

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

#endif // MYMUDUO_NET_TIMER_H