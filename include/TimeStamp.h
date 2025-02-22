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

class TimeStamp
{
public:
    using TimeDuration = std::chrono::system_clock::duration;
    using TimePoint = std::chrono::system_clock::time_point;

public:


    /**
     * @brief 用当前时间初始化对象
     */
    TimeStamp() : _M_sec(std::chrono::system_clock::now()) { }

    /**
     * @brief 用指定时间初始化对象
     * @param sec TimePoint
     */
    explicit TimeStamp(TimePoint sec) : _M_sec(sec) { }

    /**
     * @brief 获取当前时间戳
     */
    static TimeStamp now() {
        return TimeStamp();
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

private:
    // 从1970起
    TimePoint _M_sec;
};

#endif // TIMESTAMP_H