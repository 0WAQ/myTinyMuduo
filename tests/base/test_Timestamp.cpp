#include "mymuduo/base/Timestamp.h"

#include <string>
#include <thread>
#include <chrono>
#include <ctime>

#include <gtest/gtest.h>

using namespace mymuduo;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace {

// TAG: 测试默认构造函数
TEST(TimestampTest, DefaultConstructor) {
    Timestamp ts;
    EXPECT_FALSE(ts.valid()) << "Default timestamp should be invalid";
    EXPECT_EQ(ts.time_since_epoch().count(), 0);
}


// TAG: 测试当前时间获取
TEST(TimestampTest, Now) {
    Timestamp now1 = Timestamp::now();
    std::this_thread::sleep_for(10ms);
    Timestamp now2 = Timestamp::now();
    
    EXPECT_TRUE(now1.valid());
    EXPECT_TRUE(now2.valid());
    EXPECT_GT(now2, now1);
    
    // 验证时间点与系统时间一致
    auto system_now = system_clock::now();
    auto ts_now = now1.to_time_point();
    auto diff = duration_cast<milliseconds>(system_now - ts_now).count();
    EXPECT_LT(std::abs(diff), 100) << "Timestamp::now() differs from system time";
}


// TAG: 测试无效时间戳
TEST(TimestampTest, Invalid) {
    Timestamp invalid = Timestamp::invalid();
    EXPECT_FALSE(invalid.valid());
    EXPECT_EQ(invalid.time_since_epoch().count(), 0);
}


// TAG: 测试时间点构造函数
TEST(TimestampTest, TimePointConstructor) {
    auto tp = system_clock::now() + 1h;
    Timestamp ts(tp);
    
    EXPECT_TRUE(ts.valid());
    EXPECT_EQ(ts.to_time_point(), tp);
}


// TAG: 测试时间间隔构造函数
TEST(TimestampTest, DurationConstructor) {
    auto dur = 1h + 30min;
    Timestamp ts(dur);
    
    EXPECT_TRUE(ts.valid());
    EXPECT_EQ(ts.time_since_epoch(), dur);
}


// TAG: 测试时间有效性
TEST(TimestampTest, Valid) {
    Timestamp valid(Timestamp::now());
    Timestamp invalid;

    EXPECT_TRUE(valid.valid());
    EXPECT_FALSE(invalid.valid());
    
    // 负时间点应该有效
    Timestamp negative(duration_cast<system_clock::duration>(-1h));
    EXPECT_TRUE(negative.valid());
}


// TAG: 测试时间运算
TEST(TimestampTest, ArithmeticOperations) {
    Timestamp base(1h);
    auto dur = 30min;
    
    // 加法
    Timestamp added = base + dur;
    EXPECT_EQ(added.time_since_epoch(), base.time_since_epoch() + dur);
    
    // 减法
    Timestamp subtracted = base - dur;
    EXPECT_EQ(subtracted.time_since_epoch(), base.time_since_epoch() - dur);
    
    // 时间差
    Timestamp later = base + 1h;
    auto diff = later - base;
    EXPECT_EQ(diff, 1h);
}


// TAG: 测试比较操作符
TEST(TimestampTest, ComparisonOperators) {
    Timestamp t1(1h);
    Timestamp t2(2h);
    Timestamp t3(2h);
    
    EXPECT_LT(t1, t2);
    EXPECT_GT(t2, t1);
    EXPECT_LE(t1, t2);
    EXPECT_LE(t2, t3);
    EXPECT_GE(t2, t1);
    EXPECT_GE(t2, t3);
    EXPECT_EQ(t2, t3);
    EXPECT_NE(t1, t2);
}


// TAG: 测试时间点转换
TEST(TimestampTest, TimePointConversion) {
    Timestamp now = Timestamp::now();
    auto tp = now.to_time_point();
    
    Timestamp from_tp(tp);
    EXPECT_EQ(now, from_tp);
}


// TAG: 测试time_t转换
TEST(TimestampTest, TimeTConversion) {
    Timestamp now = Timestamp::now();
    time_t t = now.to_time_t();
    
    // 验证转换正确性
    auto system_time = system_clock::from_time_t(t);
    auto diff = duration_cast<seconds>(now.to_time_point() - system_time).count();
    EXPECT_LT(std::abs(diff), 1) << "Time_t conversion has significant error";
}


// TAG: 测试字符串转换
TEST(TimestampTest, ToString) {
    Timestamp now = Timestamp::now();
    
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

    EXPECT_EQ(now.to_string(), std::string(buf));
}


// TAG: 测试时间差函数
TEST(TimestampTest, TimeDifference) {
    Timestamp t1(1h);
    Timestamp t2(2h);
    
    auto diff = time_difference(t2, t1);
    EXPECT_EQ(diff, duration_cast<nanoseconds>(1h).count());
    
    auto negative_diff = time_difference(t1, t2);
    EXPECT_EQ(negative_diff, duration_cast<nanoseconds>(-1h).count());
}


// TAG: 测试添加时间函数
TEST(TimestampTest, AddTime) {
    Timestamp base(1h);
    auto dur = 30min;
    
    Timestamp result = add_time(base, dur);
    EXPECT_EQ(result.time_since_epoch(), base.time_since_epoch() + dur);
}


// TAG: 测试边界值
TEST(TimestampTest, BoundaryValues) {
    // 最小时间点
    Timestamp min_ts(system_clock::time_point::min());
    EXPECT_TRUE(min_ts.valid());
    
    // 最大时间点
    Timestamp max_ts(system_clock::time_point::max());
    EXPECT_TRUE(max_ts.valid());
    
    // 零值
    Timestamp zero(duration_cast<system_clock::duration>(0s));
    EXPECT_FALSE(zero.valid());
    EXPECT_EQ(zero.time_since_epoch().count(), 0);
    
    // 负值
    Timestamp negative(duration_cast<system_clock::duration>(-1h));
    EXPECT_TRUE(negative.valid());
    EXPECT_LT(negative, zero);
}


// TAG: 测试时间精度
TEST(TimestampTest, Precision) {
    Timestamp t1 = Timestamp::now();
    std::this_thread::sleep_for(100ns);
    Timestamp t2 = Timestamp::now();
    
    auto diff = t2 - t1;
    EXPECT_GT(diff.count(), 0);
    EXPECT_LT(diff.count(), duration_cast<nanoseconds>(1ms).count());
}


// TAG: 测试相等性
TEST(TimestampTest, Equality) {
    Timestamp t1(1h);
    Timestamp t2(1h);
    Timestamp t3(1h + 1ns);
    
    EXPECT_EQ(t1, t2);
    EXPECT_NE(t1, t3);
    
    // 相同时间点不同对象
    Timestamp t4(t1.to_time_point());
    EXPECT_EQ(t1, t4);
}


// TAG: 测试时间间隔类型
TEST(TimestampTest, DurationTypes) {
    Timestamp t1(1s);
    Timestamp t2(1000ms);
    Timestamp t3(1000000us);
    Timestamp t4(1000000000ns);
    
    EXPECT_EQ(t1, t2);
    EXPECT_EQ(t1, t3);
    EXPECT_EQ(t1, t4);
}

} // namespace

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}