#include "mymuduo/base/Timestamp.h"

using namespace mymuduo;

Timestamp::Timestamp()
    : _M_sec(TimePoint())
{ }

Timestamp::Timestamp(TimePoint sec)
    : _M_sec(sec)
{ }

Timestamp::Timestamp(TimeDuration dur)
    : _M_sec(TimePoint() + dur)
{ }

Timestamp Timestamp::now() {
    return Timestamp(std::chrono::system_clock::now());
}

Timestamp Timestamp::invalid() {
    return Timestamp();
}

std::string Timestamp::to_string() const {
    std::time_t t = to_time_t();
    std::tm* now_tm = std::localtime(&t);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);
    
    return buf;
}