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

    TimeDuration(uint64_t t, ratio r);
    TimeDuration(std::chrono::high_resolution_clock::duration d);

    DEF()
    DEF(milli)
    DEF(micro)
    DEF(nano)

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

