#ifndef MYMUDUO_NET_TIMERID_H
#define MYMUDUO_NET_TIMERID_H

#include <sys/types.h>

namespace mymuduo {
namespace net {

class Timer;

class TimerId {
public:
    friend class TimerQueue;

public:
    TimerId() : timer(nullptr), id(0) { }
    TimerId(Timer *timer, int64_t id) : timer(timer), id(id) { }

    bool operator< (const TimerId &rhs) const {
        if(this->timer == rhs.timer) {
            return this->id < rhs.id;
        }
        return this->timer < rhs.timer;
    }

private:
    Timer* timer;
    int64_t id;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TIMERID_H
