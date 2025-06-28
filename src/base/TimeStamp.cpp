#include "base/TimeStamp.h"

using namespace mymuduo;

TimeStamp::TimeStamp()
    : _M_sec(TimePoint{})
{ }

TimeStamp::TimeStamp(TimePoint sec)
    : _M_sec(sec)
{ }

TimeStamp::TimeStamp(TimeDuration dur)
    : _M_sec(TimePoint{} + dur)
{ }

TimeStamp TimeStamp::now() {
    return TimeStamp(std::chrono::system_clock::now());
}

TimeStamp TimeStamp::invalid() {
    return TimeStamp{};
}

bool TimeStamp::valid() {
    return time_since_epoch().count() > 0; 
}

TimeDuration TimeStamp::time_since_epoch() {
    return _M_sec.time_since_epoch();
}

time_t TimeStamp::to_time_t() const {
    return std::time_t(std::chrono::duration_cast<std::chrono::seconds>
        (_M_sec.time_since_epoch()).count());
}

TimePoint TimeStamp::to_time_point() const {
    return _M_sec;
}

std::string TimeStamp::to_string() const {
    std::time_t t = to_time_t();
    std::tm* now_tm = std::localtime(&t);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);
    
    return buf;
}