#pragma once

#include <iostream>
#include <string>
#include <chrono>

#include "TimeDuration.hpp"

class TimeDuration;

/**
 * 
 * 时间戳类
 * 
 */
class TimeStamp
{
public:


    /// @brief 用当前时间初始化对象
    TimeStamp();


    /// @brief 用指定时间初始化对象  
    /// @param sec std::chrono::high_resolution_clock::time_point
    TimeStamp(std::chrono::high_resolution_clock::time_point sec);


    /// @brief 获取当前时间对象
    /// @return 当前时间
    static TimeStamp now();


    /// @brief 返回当前时间点的整数表示时间
    /// @return time_t
    time_t to_time_t() const;


    /// @brief  返回当前时间点  
    /// @return std::chrono::system_clock::time_point
    std::chrono::system_clock::time_point to_time_point() const;


    /// @brief  返回当前对象的字符串表示时间 
    /// @return std::string, 格式为 yyyy-mm-dd hh24:mi:ss
    std::string to_string() const;


    ~TimeStamp();

private:
    // 从1970起
    std::chrono::high_resolution_clock::time_point _M_sec;
};