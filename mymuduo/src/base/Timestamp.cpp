#include "mymuduo/base/Timestamp.h"
#include <chrono>
#include <cstdio>
#include <ctime>

using namespace mymuduo;

Timestamp::Timestamp()
    : _sec(TimePoint())
{ }

Timestamp::Timestamp(TimePoint sec)
    : _sec(sec)
{ }

Timestamp::Timestamp(TimeDuration dur)
    : _sec(TimePoint() + dur)
{ }

Timestamp Timestamp::now() {
    return Timestamp(std::chrono::system_clock::now());
}

Timestamp Timestamp::invalid() {
    return Timestamp();
}

std::string Timestamp::to_string() const {
    using namespace std::chrono;

    auto duration = _sec.time_since_epoch();
    auto sec = duration_cast<seconds>(duration);
    auto micro = duration_cast<microseconds>(duration - sec);

    std::time_t t = system_clock::to_time_t(system_clock::time_point(sec));
    std::tm* tm = std::localtime(&t);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06lld",
                  tm->tm_year + 1900,
                  tm->tm_mon + 1,
                  tm->tm_mday,
                  tm->tm_hour,
                  tm->tm_min,
                  tm->tm_sec,
                  static_cast<long long>(micro.count()));
    return buf;
}