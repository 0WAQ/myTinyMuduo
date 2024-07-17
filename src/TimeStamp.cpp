#include "../include/TimeStamp.hpp"

TimeStamp::TimeStamp() : _M_sec(std::chrono::system_clock::now()) { }
TimeStamp::TimeStamp(TimePoint sec) : _M_sec(sec) { }
TimeStamp::~TimeStamp() { }

// 获取当前时间戳
TimeStamp TimeStamp::now() { return TimeStamp();}

// 将时间戳转换为time_t
time_t TimeStamp::to_time_t() const { return std::chrono::system_clock::to_time_t(_M_sec);}

// 将时间戳转换为chrono中的time_point
TimeStamp::TimePoint TimeStamp::to_time_point() const { return _M_sec;}

// 将时间戳转换为字符串对应的格式
std::string TimeStamp::to_string() const
{
    std::time_t t = to_time_t();
    std::tm* now_tm = std::localtime(&t);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);

    return buf;
}
