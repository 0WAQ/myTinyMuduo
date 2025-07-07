#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/Timer.h"

using namespace std::chrono_literals;

using namespace mymuduo;
using namespace mymuduo::net;

std::atomic<ssize_t> Timer::_last_timerId;

Timer::Timer(Timestamp when, TimeDuration interval, TimerCallback cb) :
        _expiration(when), _interval(interval),
        _repeat(interval > 0ns), _id(this, _last_timerId++),
        _func(cb)
{ }

void Timer::restart(Timestamp now)
{
    if(_repeat) {
        _expiration = add_time(now, _interval);
    }
    else {
        _expiration = Timestamp::invalid();
    }
}

