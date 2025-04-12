#include "Timer.h"

namespace mymuduo
{

std::atomic<ssize_t> Timer::_M_last_timerId;

Timer::Timer(TimeStamp when, double interval, TimerCallback cb) :
        _M_expiration(when), _M_interval(interval),
        _M_repeat(interval > 0.0), _M_id(this, _M_last_timerId++),
        _M_func(cb)
{ }

void Timer::restart(TimeStamp now)
{
    if(_M_repeat) {
        _M_expiration = add_time(now, _M_interval);
    }
    else {
        _M_expiration = TimeStamp::invalid();
    }
}

} // namespace mymuduo