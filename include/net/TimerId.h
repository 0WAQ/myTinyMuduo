#ifndef TIMERID_H
#define TIMERID_H

#include <sys/types.h>

namespace mymuduo {
namespace net {

class Timer;

/**
 * @brief
 */
class TimerId
{
public:

    TimerId() : timer(nullptr), id(0) { }

    TimerId(Timer *timer, int64_t id) : timer(timer), id(id) { }

    friend class TimerQueue;

    bool operator< (const TimerId &rhs) const {
        if(this->timer == rhs.timer) {
            return this->id < rhs.id;
        }
        return this->timer < rhs.timer;
    }

private:

    // 所属定时器
    Timer* timer;

    // 定时器id
    int64_t id;
};

} // namespace net
} // namespace mymuduo

#endif // TIMERID_H
