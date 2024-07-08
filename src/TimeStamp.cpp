#include "../include/TimeStamp.hpp"

TimeStamp::TimeStamp() : _M_sec(time(0))
{

}

TimeStamp::TimeStamp(int64_t sec) : _M_sec(sec)
{

}

TimeStamp TimeStamp::now() {
    return TimeStamp();
}

time_t TimeStamp::to_time_t() const {
    return _M_sec;
}

std::string TimeStamp::to_string() const
{
    char buf[32] = {0};
    tm* tm_time = localtime(&_M_sec);
    snprintf(buf, 32, "%4d-%02d-%02d %02d:%02d:%02d", 
            tm_time->tm_year + 1900,
            tm_time->tm_mon + 1,
            tm_time->tm_mday,
            tm_time->tm_hour,
            tm_time->tm_min,
            tm_time->tm_sec);
    return buf;
}

TimeStamp::~TimeStamp() {

}

