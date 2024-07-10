#include "../include/TimeStamp.hpp"

TimeStamp::TimeStamp() : _M_sec(std::chrono::high_resolution_clock::now())
{

}

TimeStamp::TimeStamp(std::chrono::high_resolution_clock::time_point sec) : _M_sec(sec)
{

}

TimeStamp TimeStamp::now() {
    return TimeStamp();
}

time_t TimeStamp::to_time_t() const {
    return std::chrono::system_clock::to_time_t(_M_sec);
}

std::chrono::system_clock::time_point TimeStamp::to_time_point() const {
    return _M_sec;
}

std::string TimeStamp::to_string() const
{
    std::time_t t = to_time_t();
    std::tm* now_tm = std::localtime(&t);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);

    return buf;
}

TimeStamp::~TimeStamp() {

}

