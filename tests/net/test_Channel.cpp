#include "base/TimeStamp.h"
#include "base/CurrentThread.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/SocketOps.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <gtest/internal/gtest-port.h>
#include <map>

#include <cstdio>
#include <memory>
#include <sys/types.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <gtest/gtest.h>

namespace {

using namespace mymuduo;
using namespace mymuduo::net;

class PeriodicTimer {
public:

    PeriodicTimer(EventLoop *loop, TimeDuration interval, const TimerCallback &cb)
        : _M_loop(loop), _M_timerfd(create_timerfd()), _M_timer_channel(loop, _M_timerfd),
            _M_interval(interval), _M_callback(cb)
    {
        _M_timer_channel.set_read_callback(std::bind(&PeriodicTimer::handle_read, this));
        _M_timer_channel.set_read_events();
    }

    void start() {
        struct itimerspec spec;
        bzero(&spec, sizeof(spec));
        spec.it_interval = to_timespec(_M_interval);
        spec.it_value = spec.it_interval;

        int ret = ::timerfd_settime(_M_timerfd, 0, &spec, NULL);
        ASSERT_TRUE(ret == 0) << "timerfd_settime() failed: " << strerror(errno);
    }

    ~PeriodicTimer() {
        _M_timer_channel.unset_all_events();
        _M_timer_channel.remove();
        sockets::close(_M_timerfd);
    }

private:

    void handle_read() {
        ASSERT_TRUE(_M_loop->is_loop_thread());
        read_timerfd(_M_timerfd);
        if(_M_callback) {
            _M_callback();
        }
    }

    static struct timespec to_timespec(TimeDuration dur) {
        struct timespec ts;
        bzero(&ts, sizeof(ts));
        const int64_t kNanoSecondsPerSecond = 1000'000'000;
        const int kMinInterval = 100'000;
        int64_t nanoseconds = dur.count();
        if(nanoseconds < kMinInterval) {
            nanoseconds = kMinInterval;
        }
        ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
        return ts;
    }

private:

    static int create_timerfd() {
        int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        EXPECT_TRUE(tfd >= 0) << "create_timerfd failed: " << strerror(errno);
        return tfd;
    }

    static void read_timerfd(int tfd) {
        uint64_t exp;
        ssize_t n = ::read(tfd, &exp, sizeof(exp));
        ASSERT_TRUE(n == sizeof(exp)) << "read timerfd " << n << " bytes instead of 8.";
    }


private:
    EventLoop *_M_loop;
    
    const int _M_timerfd;
    Channel _M_timer_channel;

    const TimeDuration _M_interval;

    TimerCallback _M_callback;
};

class ChannelTest: public ::testing::Test {
protected:
    void SetUp() override {
        loop.reset(new EventLoop);
        periodic.reset(new PeriodicTimer(loop.get(), 500ms, [this] {
            print("PeriodicTimer");
        }));
    }

    void TearDown() override {

    }

    void runWithTimeout(std::chrono::system_clock::duration timeout) {
        loop->run_after(timeout, [this] {
            loop->quit();
        });

        loop->loop();
    }

    void print(const char *msg) {
        static std::map<const char*, TimeStamp> lasts;
        TimeStamp& last = lasts[msg];
        TimeStamp now = TimeStamp::now();

        printf("\033[32m[   %s   ]\033[0m %s tid %d %s delay %f\n", "INFO", now.to_string().c_str(), CurrentThread::tid(),
            msg, 1.0 * time_difference(now, last) / TimeStamp::knaneSecondsPerSecond);
        
        last = now;
    }

protected:
    std::unique_ptr<EventLoop> loop;
    std::unique_ptr<PeriodicTimer> periodic;
};


// TAG: 测试计时器基本功能
TEST_F(ChannelTest, BasicTimer) {
    std::atomic<int> count{ 0 };

    loop->run_after(100ms, [&] {
        ++count;
        print("After Timer");
    });

    runWithTimeout(1500ms);

    EXPECT_EQ(count, 1);
}


// TAG: 测试周期性计时器
TEST_F(ChannelTest, PeriodicTimer) {
    std::atomic<int> count{ 0 };
    
    // 重置周期性回调
    periodic.reset(new PeriodicTimer(loop.get(), 200ms, [&] {
        count++;
        print("PeriodicTimer");
    }));

    periodic->start();
    
    // 运行事件循环足够时间来触发多次周期性计时器
    runWithTimeout(1s);
    
    // 检查周期性计时器被触发4-5次（每200ms一次，总时间1s）
    EXPECT_GE(count, 4);
    EXPECT_LE(count, 6);
}


// TAG: 测试多个计时器同时运行
TEST_F(ChannelTest, MultipleTimers) {
    std::atomic<int> count1{ 0 };
    std::atomic<int> count2{ 0 };
    
    loop->run_after(100ms, [&] { count1++; });
    loop->run_after(200ms, [&] { count2++; });
    
    runWithTimeout(300ms);
    
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}


// TAG: 测试计时器取消
TEST_F(ChannelTest, TimerCancel) {
    std::atomic<int> called{ 0 };
    std::atomic<int> canceled{ 0 };
    
    // 设置1秒后调用的计时器
    TimerId id = loop->run_after(500ms, [&] { called++; });
    
    // 在0.2秒后取消计时器
    loop->run_after(200ms, [&] {
        loop->cancel(id);
        canceled++;
    });
    
    runWithTimeout(700ms);
    
    EXPECT_EQ(canceled, 1);
    EXPECT_EQ(called, 0); // 计时器应该被取消，不会被调用
}

} // 匿名

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}