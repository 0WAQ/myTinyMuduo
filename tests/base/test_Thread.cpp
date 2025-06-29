#include "mymuduo/base/Thread.h"

#include <atomic>
#include <sched.h>

#include <gtest/gtest.h>
#include <thread>
#include <utility>

using namespace mymuduo;
using namespace std::chrono_literals;

namespace {


// TAG
TEST(ThreadTest, RunsFunction) {
    std::atomic<bool> executed { false };
    
    Thread thread([&] {
        executed.store(true);
    });

    thread.start();
    thread.join();

    ASSERT_TRUE(executed);
}


// TAG
TEST(ThreadTest, TidIsSet) {
    Thread thread([] { });

    thread.start();
    pid_t tid = thread.tid();
    thread.join();

    ASSERT_GE(tid, 0);
}


// TAG
TEST(ThreadTest, JoinableAndJoin) {
    Thread thread([] {
        std::this_thread::sleep_for(50ms);
    });

    ASSERT_FALSE(thread.started());
    ASSERT_FALSE(thread.joined());

    thread.start();
    ASSERT_TRUE(thread.started());
    ASSERT_TRUE(thread.joinable());

    thread.join();
    ASSERT_TRUE(thread.joined());
}


// TAG
TEST(ThreadTest, AutoNameAssigned) {
    Thread thread([] { });
    thread.start();
    thread.join();

    ASSERT_FALSE(thread.name().empty());
    ASSERT_TRUE(thread.name().find("Thread") == 0);
}


// TAG
TEST(ThreadTest, JoinBeforeStartDoesNothing) {
    Thread thread([] { });
    thread.join();

    ASSERT_FALSE(thread.joined());
    ASSERT_FALSE(thread.started());
}


// TAG
TEST(ThreadTest, DestructorDetachesIfNotJoined) {
    std::atomic<bool> finished { false };

    {
        Thread thread([&] {
            std::this_thread::sleep_for(30ms);
            finished = true;
        });
        thread.start();

        // 自动 detach
    }

    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(finished);
}


// TAG
TEST(ThreadTest, MoveSemantics) {
    std::atomic<bool> executed { false };
    Thread thread([&] {
        executed.store(true);
    });

    Thread other(std::move(thread));
    ASSERT_TRUE(thread.tid() == 0);
    ASSERT_TRUE(thread.name().empty());

    other.start();
    other.join();

    ASSERT_TRUE(executed);
}

} // namespace


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}