#pragma once

#include <chrono>
#include "TimeStamp.hpp"

enum ratio
{
    nanosecond =  1,
    microsecond = 1000 * nanosecond,
    millisecond = 1000 * microsecond,
    second = 1000 * millisecond
};

#define DEF(_TYPE_) uint64_t get_##_TYPE_##seconds();

class TimeDuration
{
public:

    /// @brief 初始化时间段
    /// @param t 时间数量
    /// @param r 时间单位
    TimeDuration(uint64_t t, ratio r);

    /// @brief 拷贝构造
    TimeDuration(std::chrono::high_resolution_clock::duration d);


    /// @brief 获取四种时间
    DEF()
    DEF(milli)
    DEF(micro)
    DEF(nano)

    /// @brief 运算符重载
    TimeDuration& operator++();
    TimeDuration& operator--();
    TimeDuration operator++(int);
    TimeDuration operator--(int);

    TimeDuration operator+(TimeDuration& rhs);
    TimeDuration operator-(TimeDuration& rhs);

    TimeDuration operator+=(TimeDuration& rhs);
    TimeDuration operator-=(TimeDuration& rhs);

    ~TimeDuration();

public:
    std::chrono::high_resolution_clock::duration _M_duration;
};

