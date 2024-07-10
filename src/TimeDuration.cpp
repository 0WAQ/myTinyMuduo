#include "../include/TimeDuration.hpp"

// 减少代码量
#define GET(_TYPE_) uint64_t TimeDuration::get_##_TYPE_##seconds() \
{ return std::chrono::duration_cast<std::chrono::_TYPE_##seconds>(_M_duration).count(); }

#define GETFUNC GET() GET(milli) GET(micro) GET(nano) 

TimeDuration::TimeDuration(uint64_t t, ratio r) : _M_duration(t * r) {}
TimeDuration::TimeDuration(std::chrono::high_resolution_clock::duration d) : _M_duration(d) {}
TimeDuration::~TimeDuration() {}

// 获取四个不同单位的时间段
GETFUNC


// ++
TimeDuration& TimeDuration::operator++() { ++_M_duration; return *this;}
TimeDuration TimeDuration::operator++(int) { return TimeDuration(_M_duration++);}


// --
TimeDuration& TimeDuration::operator--() { --_M_duration; return *this;}
TimeDuration TimeDuration::operator--(int) { return TimeDuration(_M_duration--);}


// +, -
TimeDuration TimeDuration::operator+(TimeDuration& rhs) 
{ return TimeDuration(this->_M_duration + rhs._M_duration);}

TimeDuration TimeDuration::operator-(TimeDuration& rhs) 
{ return TimeDuration(this->_M_duration - rhs._M_duration);}


// +=, -=
TimeDuration TimeDuration::operator+=(TimeDuration& rhs) { 
    _M_duration += rhs._M_duration;
    return *this;
}

TimeDuration TimeDuration::operator-=(TimeDuration& rhs) {
    _M_duration -= rhs._M_duration;
    return *this;
}
