#include "mymuduo/base/AsyncLogging.h"
#include "mymuduo/base/Logger.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace mymuduo;

void run_in_process(std::function<void()> func) {
    pid_t pid = fork();
    ASSERT_NE(pid, -1);

    if (pid == 0) {
        func();
        std::exit(0);
    }
    else {
        int status = 0;
        waitpid(pid, &status, 0);
        ASSERT_TRUE(WIFEXITED(status)) << "Child did not exit cleanly";
        ASSERT_EQ(WEXITSTATUS(status), 0) << "Child exited with error";
    }
}


namespace {

// TAG
TEST(LoggerTest, LogLevelFiltering) {
    run_in_process([] {
        std::string captured;
        Logger::instance().set_log_level(Logger::WARN);
        ASSERT_TRUE(Logger::instance().set_output([&](const char* data, size_t len) {
            captured.assign(data, len);
        }));

        LOG_DEBUG("should not appear.");
        LOG_INFO("should not appear.");
        LOG_WARN("should appear.");

        EXPECT_TRUE(captured.find("should appear") != std::string::npos);
        EXPECT_TRUE(captured.find("[WARN ]") != std::string::npos);
    });
}


// TAG
TEST(LoggerTest, WriteUseCustomBuffer) {
    run_in_process([] {
        std::string captured;
        Logger::instance().set_log_level(Logger::DEBUG);
        ASSERT_TRUE(Logger::instance().set_output([&](const char* data, size_t len) {
            captured.assign(data, len);
        }));

        LOG_INFO("Custom sink test {}.", 42);
        EXPECT_TRUE(captured.find("Custom sink test 42") != std::string::npos);
        EXPECT_TRUE(captured.find("[INFO ]") != std::string::npos);
    });
}


// TAG
TEST(LoggerTest, WriteToDevNull) {
    run_in_process([] {
        Logger::instance().set_log_level(Logger::DEBUG);
        ASSERT_TRUE(Logger::instance().set_output([&](const char* data, size_t len) {
            FILE* fp = std::fopen("/dev/null", "w+");
            std::fprintf(fp, data, len);
            std::fputc('\n', fp);
        }));

        LOG_INFO("This goes to /dev/null");
    });
}


// TAG
TEST(LoggerTest, WriteToAsyncLogging) {
    run_in_process([&] {
        std::shared_ptr<AsyncLogging> async(new AsyncLogging("./.tmp", "LoggerTest"));
        Logger::instance().set_log_level(Logger::INFO);
        ASSERT_TRUE(Logger::instance().set_async(async));

        for (int i = 0; i < 10; ++i) {
            LOG_DEBUG("this is debug{}", i);
            LOG_INFO("this is info{}", i);
            LOG_WARN("this is warn{}", i);
        }
    });
}


// TAG
TEST(LoggerTest, MultiThreadedLogging) {
    run_in_process([] {
        std::atomic<int> counter { 0 };
        Logger::instance().set_log_level(Logger::INFO);
        ASSERT_TRUE(Logger::instance().set_output([&](const char* data, size_t len) {
            ++counter;
        }));

        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < 100; ++j) {
                    LOG_INFO("Thread Logger::instance() {}.", j);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        ASSERT_EQ(1000, counter.load());
    });
}

} // namespace


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}