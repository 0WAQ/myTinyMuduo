#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "net/EventLoopThreadPool.h"

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>
#include <gtest/gtest-death-test.h>

namespace {
    
using namespace mymuduo;
using namespace mymuduo::net;

class EventLoopThreadPoolTest : public ::testing::Test {
protected:

    EventLoopThreadPoolTest()
        : _mainLoop(new EventLoop)
        , _pool(new EventLoopThreadPool(_mainLoop.get(), "test_pool"))
        , _callbackCount(0)
    { }

    ~EventLoopThreadPoolTest() {
        _pool->stop();
        _mainLoop->quit();
    }

    void SetUp() override {

    }

    void TearDown() override {

    }

protected:
    std::unique_ptr<EventLoop> _mainLoop;
    std::unique_ptr<EventLoopThreadPool> _pool;
    std::atomic<int> _callbackCount;
};


// 辅助函数
void print(EventLoop* loop) {
    printf("\033[32m[   INFO   ]\033[0m workers: pid = %d, tid = %d, loop = %p\n",
            getpid(), CurrentThread::tid(), loop);
}

// TAG: 验证线程数为 0 时只使用主循环
TEST_F(EventLoopThreadPoolTest, ZeroThreads) {
    // 设置线程数为 0
    _pool->set_thread_num(0);
    _pool->start(print);
    
    ASSERT_TRUE(_pool->started());
    
    // 验证获取的循环是主循环
    EventLoop* loop = _pool->get_next_loop();
    ASSERT_EQ(_mainLoop.get(), loop);
    
    // 多次获取应返回相同的循环
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(_mainLoop.get(), _pool->get_next_loop());
    }
    
    // 验证循环池名称
    ASSERT_STREQ("test_pool", _pool->name().c_str());
}


// TAG: 验证单线程模式
TEST_F(EventLoopThreadPoolTest, SingleThread) {
    _pool->set_thread_num(1);
    _pool->start(print);

    ASSERT_TRUE(_pool->started());
    
    // 获取第一个循环
    EventLoop* firstLoop = _pool->get_next_loop();
    ASSERT_NE(nullptr, firstLoop);
    ASSERT_NE(_mainLoop.get(), firstLoop);

    // 多次获取应返回相同的循环
    for (int i = 0; i < 5; i++) {
        EventLoop* nextLoop = _pool->get_next_loop();
        ASSERT_EQ(firstLoop, nextLoop);
    }
    
    // 验证循环池大小
    ASSERT_EQ(1, _pool->num_threads());
}


// TAG: 验证多线程轮询分配
TEST_F(EventLoopThreadPoolTest, MultipleThreadsRoundRobin) {
    const int NumThreads = 3;
    _pool->set_thread_num(NumThreads);
    _pool->start(print);
    
    // 收集所有循环
    std::vector<EventLoop*> loops;
    for (int i = 0; i < NumThreads; i++) {
        EventLoop* loop = _pool->get_next_loop();
        ASSERT_NE(nullptr, loop);
        ASSERT_NE(_mainLoop.get(), loop);
        
        // 所有循环应互不相同
        for (EventLoop* existing : loops) {
            ASSERT_NE(existing, loop);
        }
        loops.push_back(loop);
    }
    
    // 验证轮询分配
    for (int i = 0; i < 10; i++) {
        EventLoop* expected = loops[i % NumThreads];
        ASSERT_EQ(expected, _pool->get_next_loop());
    }
    
    // 检查是否所有线程都已初始化
    ASSERT_EQ(NumThreads, loops.size());
}


// TAG: 验证线程初始化回调
TEST_F(EventLoopThreadPoolTest, ThreadInitialization) {
    std::atomic<int> initCount{0};
    
    _pool->set_thread_num(3);
    _pool->start([&](EventLoop* loop) {
        print(loop);
        initCount++;
    });

    // 等待所有初始化完成
    int waitCount = 0;
    while (initCount.load() < 3 && waitCount++ < 50) {
        usleep(10000); // 10ms
    }

    ASSERT_EQ(3, initCount.load());
}


// TAG: 验证任务在正确线程执行
TEST_F(EventLoopThreadPoolTest, RunTasksInThreads) {
    constexpr int NumThreads = 5;
    const int TasksPerThread = 5;

    _pool->set_thread_num(NumThreads);
    _pool->start(print);

    // 创建任务计数器
    std::array<std::atomic<int>, NumThreads> taskCounters;
    std::ranges::fill(taskCounters, 0);

    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> taskPending { NumThreads * TasksPerThread };

    // 在每个线程中运行任务
    for (int i = 0; i < NumThreads; i++) {
        EventLoop* loop = _pool->get_next_loop();
        for (int j = 0; j < TasksPerThread; j++) {
            loop->run_in_loop([&, i] {
                taskCounters[i]++;
                _callbackCount++;
                if (--taskPending == 0) {
                    cv.notify_one();
                }
            });
        }
    }

    // 等待所有任务完成
    {
        std::unique_lock<std::mutex> lock { mtx };
        cv.wait(lock, [&] {
            return taskPending == 0;
        });
    }


    ASSERT_EQ(NumThreads * TasksPerThread, _callbackCount.load());
    for (int i = 0; i < NumThreads; i++) {
        ASSERT_EQ(TasksPerThread, taskCounters[i].load());
    }
}


// TAG: 验证多线程并发访问
TEST_F(EventLoopThreadPoolTest, ConcurrentAccess) {
    const int NumThreads = 4;
    const int TimesPerThread = 100;
    const int Total = NumThreads * TimesPerThread;

    _pool->set_thread_num(NumThreads);
    _pool->start(print);

    // 创建多个线程并发访问
    std::vector<std::thread> workers;
    std::atomic<int> total { 0 };
    std::atomic<int> taskPending { Total };
    std::mutex mtx;
    std::condition_variable cv;

    // worksers 线程向 subLoops 线程提交任务
    for (int i = 0; i < NumThreads; i++) {
        workers.emplace_back([&, this] {
            for (int i = 0; i < TimesPerThread; ++i) {
                EventLoop *loop = nullptr;
                {
                    std::lock_guard<std::mutex> guard { mtx };
                    loop = _pool->get_next_loop();
                }
                ASSERT_NE(nullptr, loop);

                total++;

                // 在循环上运行简单任务
                loop->run_in_loop([&, this] {
                    _callbackCount++;
                    if (--taskPending == 0) {
                        cv.notify_one();
                    }
                });
            }
        });
    }

    // 等待所有工作线程完成
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    // 等待任务执行完成
    {
        std::unique_lock<std::mutex> lock { mtx };
        cv.wait(lock, [&] {
            return taskPending == 0;
        });
    }

    // 验证访问计数
    ASSERT_EQ(Total, total.load());
    ASSERT_EQ(Total, _callbackCount.load());
}

} // 匿名

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    return RUN_ALL_TESTS();
}