#include "mymuduo/base/LogFile.h"
#include "mymuduo/base/Timestamp.h"

#include <cstddef>
#include <thread>
#include <fstream>
#include <iterator>
#include <filesystem>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace mymuduo;

namespace fs = std::filesystem;

namespace {

class LogFileTest : public ::testing::Test {
protected:
    LogFileTest()
        : _temp_dir(fs::temp_directory_path() / "LogFileTestDir")
        , _basename(::testing::UnitTest::GetInstance()->current_test_info()->name())
    {
        if (fs::exists(_temp_dir)) {
            fs::remove_all(_temp_dir);
        }
    }

    void TearDown() override {
        fs::remove_all(_temp_dir);
    }

    std::string read_log_content(const fs::path& path) {
        std::ifstream file(path);
        return std::string {
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }

    size_t count_log_files() {
        size_t count = 0;
        for (const auto& entry : fs::directory_iterator(_temp_dir)) {
            if (entry.path().extension() == ".log") {
                ++count;
            }
        }
        return count;
    }

protected:
    const fs::path _temp_dir;
    const std::string _basename;
};


// TAG: 测试基本日志写入
TEST_F(LogFileTest, BasicLogging) {
    LogFile logFile(_temp_dir, _basename, 
                    10 * 1024 * 1024, 
                    false, 
                    1s,
                    1024);
    
    const std::string testMessage = "Test log message\n";
    logFile.append(testMessage.data(), testMessage.size());
    logFile.flush();
    
    ASSERT_EQ(count_log_files(), 1);
    for (const auto& entry : fs::directory_iterator(_temp_dir)) {
        if (entry.path().extension() == ".log") {
            std::string content = read_log_content(entry.path());
            EXPECT_EQ(content, testMessage);
        }
    }
}


// TAG: 测试文件滚动 (基于大小)
TEST_F(LogFileTest, SizeBasedRolling) {
    const off_t rollSize = 100; // 100 Bytes
    
    LogFile logFile(_temp_dir, _basename, rollSize, 
                   false, 60s, 1024);
    
    // 写入不触发滚动的数据
    logFile.append("Short message\n", 14);
    logFile.flush();
    ASSERT_EQ(count_log_files(), 1);
    
    // 写入达到滚动阈值的数据 (该数据仍然会写入上一个文件)
    std::string longMessage(rollSize + 1, 'x');
    logFile.append(longMessage.data(), longMessage.size());
    logFile.flush();

    // 验证滚动发生
    int retry = 10;
    while (retry-- > 0 && count_log_files() < 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_EQ(count_log_files(), 2);
}


// TAG: 测试文件滚动 (基于时间)
TEST_F(LogFileTest, TimeBasedRolling) {
    LogFile logFile(_temp_dir, _basename, 
                   100 * 1024 * 1024,
                   false, 
                   0s,
                   1);

    logFile.append("Initial message\n", 16);

    for (int i = 0; i < 3; i++) {
        logFile.append("Next message\n", 13);
    }

    EXPECT_GE(count_log_files(), 1);
    EXPECT_LE(count_log_files(), 2);

    EXPECT_TRUE(logFile.roll_file());
    EXPECT_GE(count_log_files(), 2);
    EXPECT_LE(count_log_files(), 3);
}


// TAG: 测试线程安全
TEST_F(LogFileTest, ThreadSafety) {
    const int threadCount = 4;
    const int messagesPerThread = 1000;
    std::string msg = "Thread log message \n";

    // 每条消息长度为 20Bytes
    constexpr size_t expectedCount = threadCount * messagesPerThread;
    constexpr size_t expectedSize = expectedCount * 20;
    constexpr size_t rollSize = 10 * 1024 * 1024;

    LogFile logFile(_temp_dir, _basename, 
                   rollSize,
                   true,
                   1s,
                   100);

    std::vector<std::thread> threads;
    std::atomic<int> messagesCount { 0 };

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&logFile, &messagesCount, msg] {
            for (int i = 0; i < messagesPerThread; ++i) {
                logFile.append(msg.data(), msg.size());
                messagesCount++;

                if (i == messagesPerThread - 1) {
                    logFile.flush();
                }
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 验证所有消息都写入
    size_t totalSize = 0;
    for (const auto& entry : fs::directory_iterator(_temp_dir)) {
        if (entry.path().extension() == ".log") {
            totalSize += fs::file_size(entry.path());
        }
    }

    EXPECT_EQ(messagesCount, expectedCount);
    EXPECT_EQ(totalSize, expectedSize);
}


// TAG: 测试刷新机制
TEST_F(LogFileTest, FlushMechanism) {
    LogFile logFile(_temp_dir, _basename, 
                   100 * 1024 * 1024, 
                   false, 
                   1s,
                   1); // 每次写入后检查
    Timestamp t = Timestamp();
    logFile.append("Message without flush\n", 22);

    // 验证文件为空（未刷新）
    for (const auto& entry : fs::directory_iterator(_temp_dir)) {
        if (entry.path().extension() == ".log") {
            std::string content = read_log_content(entry.path());
            EXPECT_TRUE(content.empty());
        }
    }

    logFile.flush();
    
    // 验证内容已写入
    for (const auto& entry : fs::directory_iterator(_temp_dir)) {
        if (entry.path().extension() == ".log") {
            std::string content = read_log_content(entry.path());
            EXPECT_EQ(content, "Message without flush\n");
        }
    }
    
    // 测试自动刷新
    std::this_thread::sleep_for(std::chrono::seconds(2));
    logFile.append("Next message\n", 13);
    
    // 验证内容已自动刷新
    for (const auto& entry : fs::directory_iterator(_temp_dir)) {
        if (entry.path().extension() == ".log") {
            std::string content = read_log_content(entry.path());
            EXPECT_EQ(content, "Message without flush\nNext message\n");
        }
    }
}


// TAG: 测试多次滚动
TEST_F(LogFileTest, MultipleRollovers) {
    const off_t rollSize = 100;
    
    LogFile logFile(_temp_dir, _basename, rollSize, 
                   false, 60s, 1);
    
    // 连续触发多次滚动
    for (int i = 0; i < 5; i++) {
        std::string message(rollSize + 1, 'a' + i);
        logFile.append(message.data(), message.size());
    }

    // 验证日志文件数量
    EXPECT_EQ(count_log_files(), 6);
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}