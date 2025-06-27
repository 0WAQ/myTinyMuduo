#include "base/ThreadPool.h"

#include <atomic>
#include <thread>

#include <gtest/gtest.h>

using namespace mymuduo;
using namespace std::chrono_literals;

namespace {


// TAG
TEST(ThreadPoolTest, SingleTaskExecuted) {
    ThreadPool pool("TEST", 1);
    std::atomic<bool> flag { false };

    pool.push([&] {
        flag.store(true);
    });

    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(flag);

    pool.stop();
}


// TAG
TEST(ThreadPoolTest, MultipleTasksExecuted) {
    ThreadPool pool("TEST", 2);
    std::atomic<int> counter { 0 };

    for (int i = 0; i < 10; ++i) {
        pool.push([&] {
            ++counter;
        });
    }

    std::this_thread::sleep_for(200ms);
    ASSERT_EQ(10, counter.load());

    pool.stop();
}


// TAG
TEST(ThreadPoolTest, StopPreventsExecution) {
    ThreadPool pool("TEST", 2);
    pool.stop();

    std::atomic<bool> executed { false };
    pool.push([&] {
        executed.store(true);
    });

    std::this_thread::sleep_for(100ms);
    ASSERT_FALSE(executed.load());
}


// TAG
TEST(ThreadPoolTest, DestructorStopsThread) {
    std::atomic<int> counter { 0 };
    {
        ThreadPool pool("TEST", 4);
        for (int i = 0; i < 8; ++i) {
            pool.push([&] {
                ++counter;
            });
        }
    }

    ASSERT_EQ(8, counter.load());
}


// TAG
TEST(ThreadPoolTest, StressTestManyTasks) {
    ThreadPool pool("TEST", 8);
    std::atomic<int> counter { 0 };

    const int task_count = 100;
    for (int i = 0; i < task_count; ++i) {
        pool.push([&] {
            ++counter;
        });
    }

    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(counter.load(), task_count);

    pool.stop();
}


} // namespace


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}