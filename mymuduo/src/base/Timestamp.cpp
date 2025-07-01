#include "mymuduo/base/Timestamp.h"

using namespace mymuduo;

Timestamp::Timestamp()
    : _M_sec(TimePoint{})
{ }

Timestamp::Timestamp(TimePoint sec)
    : _M_sec(sec)
{ }

Timestamp::Timestamp(TimeDuration dur)
    : _M_sec(TimePoint{} + dur)
{ }

Timestamp Timestamp::now() {
    return Timestamp(std::chrono::system_clock::now());
}

Timestamp Timestamp::invalid() {
    return Timestamp{};
}

bool Timestamp::valid() {
    return time_since_epoch().count() > 0; 
}

TimeDuration Timestamp::time_since_epoch() {
    return _M_sec.time_since_epoch();
}

time_t Timestamp::to_time_t() const {
    return std::time_t(std::chrono::duration_cast<std::chrono::seconds>
        (_M_sec.time_since_epoch()).count());
}

TimePoint Timestamp::to_time_point() const {
    return _M_sec;
}

std::string Timestamp::to_string() const {
    std::time_t t = to_time_t();
    std::tm* now_tm = std::localtime(&t);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);
    
    return buf;
}