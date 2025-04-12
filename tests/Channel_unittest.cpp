#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <functional>
#include <map>

#include <cstdio>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace mymuduo;

void print(const char *msg)
{
    static std::map<const char*, TimeStamp> lasts;
    TimeStamp& last = lasts[msg];
    TimeStamp now = TimeStamp::now();

    printf("%s tid %d %s delay %f\n", now.to_string().c_str(), CurrentThread::tid(),
            msg, 1.0 * time_difference(now, last) / TimeStamp::knaneSecondsPerSecond);
    
    last = now;
}

namespace mymuduo
{
namespace __detail
{
    int create_timerfd();
    void read_timerfd(int timerfd, TimeStamp now);
}
}

class PeriodicTimer
{
public:

    PeriodicTimer(EventLoop *loop, double interval, const TimerCallback &cb)
        : _M_loop(loop), _M_timerfd(__detail::create_timerfd()), _M_timer_channel(loop, _M_timerfd),
          _M_interval(interval), _M_callback(cb)
    {
        _M_timer_channel.set_read_callback(std::bind(&PeriodicTimer::handle_read, this));
        _M_timer_channel.set_read_events();
    }

    void start()
    {
        struct itimerspec spec;
        bzero(&spec, sizeof(spec));
        spec.it_interval = to_timespec(_M_interval);
        spec.it_value = spec.it_interval;

        int ret = ::timerfd_settime(_M_timerfd, 0, &spec, NULL);
        if(ret) {
            printf("timerfd_settime()\n");
        }
    }

    ~PeriodicTimer()
    {
        _M_timer_channel.unset_all_events();
        _M_timer_channel.remove();
        ::close(_M_timerfd);
    }

private:

    void handle_read()
    {
        assert(_M_loop->is_loop_thread());
        __detail::read_timerfd(_M_timerfd, TimeStamp::now());
        if(_M_callback) {
            _M_callback();
        }
    }

    static struct timespec to_timespec(double seconds)
    {
        struct timespec ts;
        bzero(&ts, sizeof(ts));
        const int64_t kNanoSecondsPerSecond = 1000'000'000;
        const int kMinInterval = 100'000;
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
        if(nanoseconds < kMinInterval) {
            nanoseconds = kMinInterval;
        }
        ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
        return ts;
    }

private:

    EventLoop *_M_loop;
    
    const int _M_timerfd;
    Channel _M_timer_channel;

    const double _M_interval;

    TimerCallback _M_callback;
};

int main()
{
    printf("pid = %d, tid = %d. Try adjusting the wall clock, see what happens.", gettid(), CurrentThread::tid());

    EventLoop loop;
    PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
    timer.start();
    loop.run_every(1, std::bind(print, "EventLoop::run_every"));
    loop.loop();
}