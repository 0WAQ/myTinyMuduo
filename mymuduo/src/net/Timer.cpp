#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/Timer.h"

using namespace std::chrono_literals;

using namespace mymuduo;
using namespace mymuduo::net;

std::atomic<ssize_t> Timer::_M_last_timerId;

Timer::Timer(Timestamp when, TimeDuration interval, TimerCallback cb) :
        _M_expiration(when), _M_interval(interval),
        _M_repeat(interval > 0ns), _M_id(this, _M_last_timerId++),
        _M_func(cb)
{ }

void Timer::restart(Timestamp now)
{
    if(_M_repeat) {
        _M_expiration = add_time(now, _M_interval);
    }
    else {
        _M_expiration = Timestamp::invalid();
    }
}

