#include "EventLoopThread.h"
#include "EventLoop.h"

#include <atomic>
#include <cstdio>
#include <gtest/gtest-death-test.h>
#include <memory>
#include <unistd.h>

#include <gtest/gtest.h>

namespace {
    
using namespace mymuduo;
using namespace std::chrono_literals;

class EventLoopThreadTest : public ::testing::Test {
protected:
    EventLoopThreadTest()
        : _loop_thread(new EventLoopThread)
        , _task_executed(false)
    { }

    void SetUp() override {

    }

    void TearDown() override {
        _loop_thread->stop_loop();
    }

protected:
    std::unique_ptr<EventLoopThread> _loop_thread;
    std::atomic<bool> _task_executed;
};

// TAG: 测试正常启动
TEST_F(EventLoopThreadTest, NormalStart) {
    EventLoop* loop = _loop_thread->start_loop();
    ASSERT_NE(nullptr, loop);
    
    // 验证状态
    EXPECT_TRUE(_loop_thread->started());
    EXPECT_TRUE(_loop_thread->running());
    
    // 执行跨线程任务
    loop->run_in_loop([this] {
        _task_executed = true;
    });
    
    // 等待任务执行
    while (!_task_executed) {
        usleep(1000);
    }
    
    EXPECT_TRUE(_task_executed);
}

// TAG: 测试超时启动
TEST(EventLoopThreadTimeoutTest, TimeoutStart) {
    EventLoop* loop = nullptr;

    // 超时启动
    {
        std::unique_ptr<EventLoopThread> loop_thread { new EventLoopThread };
        ASSERT_DEATH(   // 可能会失败
            {
                // 设置1ns超时 - 实际上线程创建需要更长时间
                loop = loop_thread->start_loop(1ns);
            },
            "EventLoopThread start_loop timeout after 1 ns\n"
        );
        EXPECT_EQ(nullptr, loop);

        if (loop) {
            loop_thread->stop_loop();
        }
    }

    // 正常启动
    {
        //std::unique_ptr<EventLoopThread> loop_thread { new EventLoopThread };
        //loop_thread->start_loop(500ms);   
        //EXPECT_NE(nullptr, loop);
        //loop_thread->stop_loop();
    }
}

// TAG: 测试多次启动
TEST_F(EventLoopThreadTest, MultipleStarts) {
    EventLoop* loop1 = _loop_thread->start_loop();
    ASSERT_NE(nullptr, loop1);
    
    EventLoop* loop2 = _loop_thread->start_loop();
    EXPECT_EQ(loop1, loop2);
}

// TAG: 测试安全停止
TEST_F(EventLoopThreadTest, SafeStop) {
    EventLoop* loop = _loop_thread->start_loop();
    ASSERT_NE(nullptr, loop);
    
    // 启动监控线程
    std::thread watcher([this] {
        while (_loop_thread->running()) {
            usleep(1000);
        }
    });
    
    // 停止事件循环
    _loop_thread->stop_loop();
    
    // 等待线程结束
    watcher.join();
    
    EXPECT_FALSE(_loop_thread->running());
}

} // 匿名

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    return RUN_ALL_TESTS();
}