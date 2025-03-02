/**
 * 
 * TimeStamp头文件
 * 
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <iostream>
#include <string>
#include <chrono>

// system_clock的时间精度为ns
using TimePoint = std::chrono::system_clock::time_point;
using TimeDuration = std::chrono::system_clock::duration;

class TimeStamp
{
public:

    /**
     * @brief 用零值初始化TimeStamp
     */
    TimeStamp() : _M_sec(TimePoint{}) { }

    /**
     * @brief 用指定时间初始化对象
     * @param sec TimePoint
     */
    explicit TimeStamp(TimePoint sec) : _M_sec(sec) { }

    /**
     * @brief 获取当前时间戳
     */
    static TimeStamp now() {
        return TimeStamp(std::chrono::system_clock::now());
    }

    static TimeStamp invalid() {
        return TimeStamp{};
    }

    TimeDuration time_since_epoch() {
        return _M_sec.time_since_epoch();
    }

    /**
     * @brief 将时间戳转换为time_t
     */
    time_t to_time_t() const {
        return std::chrono::system_clock::to_time_t(_M_sec);
    }

    /**
     * @brief 将时间戳转换为chrono中的time_point
     */
    TimePoint to_time_point() const {
        return _M_sec;
    }

    /**
     * @brief 将时间戳转换为字符串对应的格式
     * @return yyyy-mm-dd hh24:mi:ss
     */
    std::string to_string() const {
        std::time_t t = to_time_t();
        std::tm* now_tm = std::localtime(&t);
    
        char buf[80];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", now_tm);
        
        return buf;
    }

    TimeStamp operator+ (const TimeDuration &duration) {
        return TimeStamp(TimePoint(this->time_since_epoch() + duration));
    }

    TimeStamp operator- (const TimeDuration &duration) {
        return TimeStamp(TimePoint(this->time_since_epoch() - duration));
    }

    bool operator< (const TimeStamp &rhs) {
        return this->_M_sec < rhs._M_sec;
    }

    bool operator> (const TimeStamp &rhs) {
        return this->_M_sec > rhs._M_sec;
    }

    bool operator== (const TimeStamp &rhs) {
        return this->_M_sec == rhs._M_sec;
    }

    bool operator<= (const TimeStamp &rhs) {
        return !(this->operator>(rhs));
    }

    bool operator>= (const TimeStamp &rhs) {
        return !(this->operator<(rhs));
    }

    bool operator!= (const TimeStamp &rhs) {
        return !(this->operator==(rhs));
    }

private:
    // 从1970起
    TimePoint _M_sec;
};

inline TimeStamp add_time(TimeStamp timestamp, double seconds) {
    using namespace std::chrono;
    auto cast_duration = duration_cast<TimeDuration>(duration<double>(seconds));
    return timestamp + cast_duration;
}

#endif // TIMESTAMP_H