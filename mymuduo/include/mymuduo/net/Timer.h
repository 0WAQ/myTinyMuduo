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

    void run() { _func(); }
    void restart(Timestamp now);

    Timestamp expiration() const { return _expiration; }
    bool repeat() const { return _repeat; }
    TimerId id() const { return _id; }
    ssize_t last_timerId() const { return _last_timerId; }

private:
    // 到期时间
    Timestamp _expiration;

    // 间隔时间
    const TimeDuration _interval;

    // 是否重复
    bool _repeat;

    // 定时器回调函数
    const TimerCallback _func;

    // 定时器唯一id
    const TimerId _id;

    static std::atomic<ssize_t> _last_timerId;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMER_H