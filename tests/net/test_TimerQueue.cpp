#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/TimerId.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/EventLoopThread.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <condition_variable>
#include <algorithm>
#include <mutex>
#include <unistd.h>

#include <gtest/gtest.h>

namespace {

using namespace mymuduo;
using namespace mymuduo::net;

class TimerQueueTest : public ::testing::Test {
protected:

    TimerQueueTest()
        : _loop(new EventLoop)
    { }

    void SetUp() override { }

    void TearDown() override {
        _loop->quit();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

protected:
    std::unique_ptr<EventLoop> _loop;
    Timestamp _lastRunTime;
    std::thread _thread;
};


// TAG: 一次性定时器测试
TEST_F(TimerQueueTest, OneShotTimers) {
    std::array<bool, 3> executed;
    executed.fill(false);

    _loop->run_after(100ms, [&] {
        executed[0] = true;
        EXPECT_FALSE(executed[1]);
        EXPECT_FALSE(executed[2]);
    });

    _loop->run_after(200ms, [&] {
        EXPECT_TRUE(executed[0]);
        executed[1] = true;
        EXPECT_FALSE(executed[2]);
    });

    _loop->run_after(300ms, [&] {
        EXPECT_TRUE(executed[0]);
        EXPECT_TRUE(executed[1]);
        executed[2] = true;
    });

    _loop->run_after(400ms, [&] {
        EXPECT_TRUE(executed[0]);
        EXPECT_TRUE(executed[1]);
        EXPECT_TRUE(executed[2]);
        executed[3] = true;
        _loop->quit();
    });

    _loop->loop();
}


// TAG: 循环定时器测试
TEST_F(TimerQueueTest, RepeatingTimer) {
    int count = 0;

    // 每 0.1s 执行一次的定时器
    TimerId timer_id = _loop->run_every(100ms, [&] {
        ++count;
    });
    
    // 设置 0.55s 后取消定时器
    _loop->run_after(550ms, [this, timer_id] {
        _loop->cancel(timer_id);
        _loop->quit();
    });
    
    _loop->loop();


    // 预期在 550ms 内执行了 5 ~ 6 次
    EXPECT_GE(count, 5);
    EXPECT_LT(count, 6);
}


// TAG: 定时器取消测试
TEST_F(TimerQueueTest, TimerCancellation) {
    bool executed1 = false;
    bool executed2 = false;
    
    TimerId timer1 = _loop->run_after(200ms, [&] {
        executed1 = true;
    });

    _loop->run_after(100ms, [&] {
        executed2 = true;
        _loop->cancel(timer1);  // 定时器 1 将被取消
        _loop->quit();
    });
    
    _loop->loop();

    EXPECT_TRUE(executed2);
    EXPECT_FALSE(executed1);
}


// TAG: 定时器精度测试
TEST_F(TimerQueueTest, TimerPrecision) {
    Timestamp start = Timestamp::now();
    _loop->run_after(200ms, [this, start] {
        Timestamp end = Timestamp::now();
        int64_t duration = time_difference(end, start) / 1e6; // ns => ms
        EXPECT_NEAR(duration, 200, 20); // 允许 ±20ms 的误差
        _loop->quit();
    });
    
    _loop->loop();
}


// TAG: 跨线程定时器测试
TEST_F(TimerQueueTest, CrossThreadTimers) {

    int count = 0;

    // 创建一个子线程去执行定时器
    std::thread another([&] {
        TimerId timer = _loop->run_every(100ms, [&] {
            ++count;
        });

        _loop->run_after(550ms, [&] {
            _loop->cancel(timer);
            _loop->quit();
        });
    });

    _loop->loop();

    if (another.joinable()) {
        another.join();
    }
    
    EXPECT_GE(count, 5);
    EXPECT_LT(count, 6);

    ///////////////////////////////////////////////////////////////////////////////

    count = 0;
    EventLoop* loop = nullptr;
    std::condition_variable cond;
    std::mutex mtx;
    std::unique_lock<std::mutex> lock { mtx };

    std::thread second([&count, &loop, &cond] {
        std::unique_ptr<EventLoop> ploop { new EventLoop };
        loop = ploop.get();
        cond.notify_one();
        loop->loop();
    });

    cond.wait(lock, [&] {
        return loop != nullptr;
    });

    TimerId timer = loop->run_every(100ms, [&] {
        ++count;
    });

    loop->run_after(550ms, [&] {
        loop->cancel(timer);
        loop->quit();
    });

    if (second.joinable()) {
        second.join();
    }

    EXPECT_GE(count, 5);
    EXPECT_LT(count, 6);
}


// TAG: 大容量定时器压力测试
TEST_F(TimerQueueTest, HighVolumeTimers) {
    const int TIMER_COUNT = 100;
    std::array<bool, TIMER_COUNT> executed;
    executed.fill(false);

    // 添加 100 个定时器，间隔 1ms
    for (int i = 0; i < TIMER_COUNT; i++) {
        _loop->run_after(std::chrono::milliseconds{ i + 1 }, [i, &executed] {
            executed[i] = true;
        });
    }

    // 等待 0.11s 后退出
    _loop->run_after(110ms, [&] {
        _loop->quit();
    });

    _loop->loop();

    int count = std::ranges::count(executed, true);
    EXPECT_EQ(TIMER_COUNT, count);
}


// TAG: 相同时间点多个定时器测试
TEST_F(TimerQueueTest, SimultaneousTimers) {
    const int TIMER_COUNT = 5;
    int count = 0;
    
    // 添加 5 个相同时间间隔的定时器
    for (int i = 0; i < TIMER_COUNT; i++) {
        _loop->run_after(200ms, [&count] {
            count++;
        });
    }

    _loop->run_after(300ms, [&] {
        _loop->quit();
    });
    
    _loop->loop();
    
    EXPECT_EQ(TIMER_COUNT, count);
}


// TAG: 长时间运行的定时器测试
TEST_F(TimerQueueTest, LongRunningTimers) {
    int count = 0;

    // 添加每 0.3s 执行一次的定时器
    TimerId timerId = _loop->run_every(300ms, [this, &count] {
        ++count;
        std::this_thread::sleep_for(50ms);  // 模拟耗时操作
    });
    
    // 2s 后取消定时器
    _loop->run_after(2s, [this, timerId] {
        _loop->cancel(timerId);
        _loop->quit();
    });
    
    _loop->loop();

    // 预期在2秒内执行了6-7次
    EXPECT_GE(count, 6);
    EXPECT_LE(count, 7);
}


// TAG: 取消已执行定时器测试
TEST_F(TimerQueueTest, CancelCompletedTimer) {
    int count = 0;
    
    // 添加定时器
    TimerId timer = _loop->run_every(10ms, [&] {
        count = true;
    });
    
    // 添加取消任务 (在定时器执行之后)
    _loop->run_after(20ms, [&] {
        EXPECT_NO_THROW(_loop->cancel(timer));
        _loop->quit();
    });
    
    _loop->loop();

    EXPECT_GE(count, 1);
    EXPECT_LE(count, 2);
}


// TAG: 在事件循环线程中添加定时器
TEST_F(TimerQueueTest, AddTimerFromLoopThread) {
    bool executed = false;
    
    // 从事件循环线程添加定时器
    _loop->run_in_loop([this, &executed] {
        _loop->run_after(10ms, [&] {
            executed = true;
        });

        _loop->run_after(20ms, [&] {
            _loop->quit();
        });
    });

    _loop->loop();
    
    EXPECT_TRUE(executed);
}

} // 匿名

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}