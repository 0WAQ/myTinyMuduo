#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/EventLoop.h"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <memory>
#include <pthread.h>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>
#include <gtest/gtest-death-test.h>

namespace {

using namespace mymuduo;
using namespace mymuduo::net;
    
class EventLoopTest : public ::testing::Test {
protected:

    EventLoopTest()
        : _loop(new EventLoop)
        , _another(nullptr)
    { }

    void SetUp() override { }

    void TearDown() override {
        _loop->quit();
        if (_another) {
            _another->quit();
        }

        if (_thread.joinable()) {
            _thread.join();
        }
    }

    void startAnotherLoop() {
        _thread = std::thread([this] {
            EventLoop loop;
            _another = &loop;
            _another->loop();
        });

        while (!_another) {
            usleep(10000);
        }
    }

protected:
    std::unique_ptr<EventLoop> _loop;
    EventLoop* _another;
    std::thread _thread;
};


// TAG: 测试事件循环在创建线程中正常工作
TEST_F(EventLoopTest, RunInCreatingThread) {
    std::atomic<bool> called { false };

    _loop->run_in_loop([&] {
        EXPECT_TRUE(_loop->is_loop_thread());
        called.store(true);
    });

    // 提前终止
    _loop->run_after(1s, [&] {
        _loop->quit();
    });

    _loop->loop_once();
    EXPECT_TRUE(called.load());
}


// TAG: 测试跨线程操作
TEST_F(EventLoopTest, CrossThreadOperation) {
    startAnotherLoop();

    std::atomic<bool> callbackCalled{ false };

    _another->run_in_loop([&] {
        EXPECT_TRUE(_another->is_loop_thread());
        callbackCalled.store(true);
    });

    // 提前退出事件循环
    _another->run_after(1s, [&] {
        _another->quit();
    });

    // 等待操作完成
    int waitCount = 0;
    while (!callbackCalled.load() && waitCount++ < 50) {
        usleep(10000);
    }

    EXPECT_TRUE(callbackCalled.load());
}


// TAG: 测试同一线程多次创建事件循环
TEST_F(EventLoopTest, SingleThreadMultiLoop) {
    ASSERT_DEATH(   // 会创建一个子进程
        {
            EventLoop secondLoop;   // 会退出
        },
        "Another EventLoop 0x[0-9a-fA-F]+ exists in this thread [0-9]+\n"
    );
}


// TAG: 测试在未运行循环时的调度操作
TEST_F(EventLoopTest, ScheduleWithoutRunningLoop) {
    std::atomic<bool> called { false };

    _loop->queue_in_loop([&] {
        called.store(true);
    });

    _loop->run_after(200ms, [&] {
        _loop->quit();
    });

    EXPECT_FALSE(called.load());

    // 调用一次循环处理
    _loop->loop_once();
    EXPECT_TRUE(called.load());
}


// TAG: 测试跨线程定时器调度
TEST_F(EventLoopTest, CrossThreadTimer) {
    startAnotherLoop();

    std::atomic<bool> timerCalled { false };
    auto start = Timestamp::now();

    // 主线程在其他线程的事件循环上设置定时器
    _another->run_after(200ms, [&] {
        timerCalled.store(true);
        _another->quit();
    });

    // 等待操作完成
    int waitCount = 0;
    while (!timerCalled.load() && waitCount++ < 50) {
        usleep(10000);
    }

    auto duration = time_difference(Timestamp::now(), start);
    EXPECT_GE(duration, 190000);
    EXPECT_TRUE(timerCalled.load());
}

} // 匿名

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}