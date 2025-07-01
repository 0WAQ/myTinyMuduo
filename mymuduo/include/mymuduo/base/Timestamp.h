/**
 * 
 * TimeStamp头文件
 * 
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>
#include <chrono>

namespace mymuduo {

// system_clock 的时间精度为ns
using TimePoint = std::chrono::system_clock::time_point;
using TimeDuration = std::chrono::system_clock::duration;

class Timestamp {
public:

    static const int kMilliSecondsPerSecond = 1000;
    static const int kMicroSecondsPerSecond = 1000 * 1000;
    static const int knaneSecondsPerSecond  = 1000 * 1000 * 1000;

public:
    explicit Timestamp();
    explicit Timestamp(TimePoint sec);
    explicit Timestamp(TimeDuration dur);

    static Timestamp now();

    static Timestamp invalid();
    bool valid();

    TimeDuration time_since_epoch();
    time_t to_time_t() const;
    TimePoint to_time_point() const;

    /**
     * @brief 将时间戳转换为字符串对应的格式
     * @return yyyy-mm-dd hh24:mi:ss
     */
    std::string to_string() const;

    Timestamp operator+ (const TimeDuration &duration) {
        return Timestamp(TimePoint(this->time_since_epoch() + duration));
    }

    Timestamp operator- (const TimeDuration &duration) {
        return Timestamp(TimePoint(this->time_since_epoch() - duration));
    }

    TimeDuration operator- (const Timestamp& other) {
        return this->_M_sec - other._M_sec;
    }

    bool operator< (const Timestamp &rhs) const {
        return this->_M_sec < rhs._M_sec;
    }

    bool operator> (const Timestamp &rhs) const {
        return this->_M_sec > rhs._M_sec;
    }

    bool operator== (const Timestamp &rhs) const {
        return this->_M_sec == rhs._M_sec;
    }

    bool operator<= (const Timestamp &rhs) const {
        return !(this->operator>(rhs));
    }

    bool operator>= (const Timestamp &rhs) const {
        return !(this->operator<(rhs));
    }

    bool operator!= (const Timestamp &rhs) const {
        return !(this->operator==(rhs));
    }

private:
    // 从1970起
    TimePoint _M_sec;
};

inline Timestamp add_time(Timestamp timestamp, TimeDuration dur) {
    return timestamp + dur;
}

inline int64_t time_difference(Timestamp high, Timestamp low) {
    return (high - low).count();
}

} // namespace mymuduo

#endif // TIMESTAMP_H