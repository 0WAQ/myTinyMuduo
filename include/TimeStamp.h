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


    /// @brief 用当前时间初始化对象
    TimeStamp();


    /// @brief 用指定时间初始化对象  
    /// @param sec TimePoint
    TimeStamp(TimePoint sec);


    /// @brief 获取当前时间对象
    /// @return 当前时间
    static TimeStamp now();


    /// @brief 返回当前时间点的整数表示时间
    /// @return time_t
    time_t to_time_t() const;


    /// @brief  返回当前时间点  
    /// @return TimePoint
    TimePoint to_time_point() const;


    /// @brief  返回当前对象的字符串表示时间 
    /// @return std::string, 格式为 yyyy-mm-dd hh24:mi:ss
    std::string to_string() const;

private:
    // 从1970起
    TimePoint _M_sec;
};

#endif // TIMESTAMP_H