#include "mymuduo/net/Buffer.h"

#include <arpa/inet.h>
#include <gtest/gtest.h>

using namespace mymuduo;
using namespace mymuduo::net;

namespace {

// TAG: 基本操作测试
TEST(BufferTest, BasicOperations) {
    Buffer buf;
    
    // 初始状态验证
    EXPECT_EQ(buf.prependable(), 8);      // 默认prependable大小
    EXPECT_EQ(buf.writable(), 1024);      // 默认writable大小
    EXPECT_EQ(buf.readable(), 0);
    
    // 添加数据
    const std::string msg = "Hello, Buffer!";
    buf.append_with_sep(msg);
    EXPECT_EQ(buf.readable(), msg.size());
    EXPECT_EQ(buf.writable(), 1024 - msg.size());
    
    // 检索部分数据
    auto part = buf.retrieve_as_string(6);
    EXPECT_EQ(part, "Hello,");
    EXPECT_EQ(buf.readable(), msg.size() - 6);
    
    // 检索剩余数据
    auto rest = buf.retrieve_all_as_string();
    EXPECT_EQ(rest, " Buffer!");
    EXPECT_EQ(buf.readable(), 0);
}


// TAG: 不同分割符策略测试
TEST(BufferTest, SeparationStrategies) {
    // 1. 测试无分割符策略
    Buffer noSepBuf;
    noSepBuf.append_with_sep("packet1_packet2_packet3");
    
    std::string packet;
    bool hasPacket = noSepBuf.pick_datagram(packet);
    EXPECT_TRUE(hasPacket);
    EXPECT_EQ(packet, "packet1_packet2_packet3");
    EXPECT_EQ(noSepBuf.readable(), 0);
    
    // 2. 测试四字节报文头策略
    Buffer headerBuf;
    headerBuf.set_sep(Buffer::LengthPrefix);
    const std::string packet1 = "HeaderTest1";
    const std::string packet2 = "HeaderTest2";

    headerBuf.append_with_sep(packet1);
    headerBuf.append_with_sep(packet2);

    std::string result;
    EXPECT_TRUE(headerBuf.pick_datagram(result));
    EXPECT_EQ(result, packet1);

    EXPECT_TRUE(headerBuf.pick_datagram(result));
    EXPECT_EQ(result, packet2);
    
    // 3. 分割符后缀
    Buffer delimiterBuf;
    delimiterBuf.set_sep(Buffer::DelimiterSuffix);
    delimiterBuf.append_with_sep("TestDelimiterSuffix1");
    delimiterBuf.append_with_sep("TestDelimiterSuffix2");

    // 提取报文体
    EXPECT_TRUE(delimiterBuf.pick_datagram(result));
    EXPECT_EQ(result, "TestDelimiterSuffix1");
    EXPECT_TRUE(delimiterBuf.pick_datagram(result));
    EXPECT_EQ(result, "TestDelimiterSuffix2");
}


// TAG: 缓冲区大小管理测试
TEST(BufferTest, BufferSizeManagement) {
    // 测试自定义初始大小
    Buffer customBuf(16, 2048);
    EXPECT_EQ(customBuf.prependable(), 16);
    EXPECT_EQ(customBuf.writable(), 2048);
    
    // 填充缓冲区
    std::string largeData(1500, 'x');
    customBuf.append_with_sep(largeData);
    EXPECT_EQ(customBuf.readable(), 1500);
    EXPECT_EQ(customBuf.writable(), 2048 - 1500);
    
    // 手动扩容
    customBuf.resize(4096);
    EXPECT_GE(customBuf.writable(), 4096 - 1500);
    
    // 确保可写空间
    customBuf.ensure_writable(3000);
    EXPECT_GE(customBuf.writable(), 3000);
}

// TAG: 数据操作测试
TEST(BufferTest, DataOperations) {
    Buffer buf;
    
    const char* raw = "Raw binary data";
    buf.append_with_sep(raw, 5);

    EXPECT_EQ(buf.retrieve_as_string(5), "Raw b");
    EXPECT_EQ(buf.readable(), 0);

    buf.append_with_sep("String data");
    EXPECT_EQ(buf.readable(), 11);
    EXPECT_EQ(buf.retrieve_as_string(7), "String ");
    
    // 删除数据
    buf.append_with_sep("To be erased");
    auto erased = buf.erase(3);
    EXPECT_EQ(erased, "dat");
    EXPECT_EQ(buf.retrieve_all_as_string(), "aTo be erased");
}


// TAG: 文件描述符操作测试
TEST(BufferTest, FileDescriptorOperations) {
    // 创建管道
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    
    // 准备写入数据
    const std::string data = "Data for file descriptor test";
    
    // 测试写入文件描述符
    Buffer buf;
    buf.append_with_sep(data);
    
    int err = 0;
    size_t written = buf.write_fd(pipefd[1], &err);
    EXPECT_EQ(written, data.size());
    EXPECT_EQ(err, 0);
    
    // 清空缓冲区
    buf.retrieve_all();
    
    // 测试从文件描述符读取
    size_t readSize = buf.read_fd(pipefd[0], &err);
    EXPECT_EQ(readSize, data.size());
    EXPECT_EQ(err, 0);
    EXPECT_EQ(buf.retrieve_all_as_string(), data);
    
    close(pipefd[0]);
    close(pipefd[1]);
}


// TAG: 边界条件测试
TEST(BufferTest, BoundaryConditions) {
    // 测试空缓冲区操作
    Buffer emptyBuf;
    EXPECT_TRUE(emptyBuf.retrieve_all_as_string().empty());
    EXPECT_EQ(emptyBuf.erase(10), "");
    
    // 测试无效提取
    std::string temp;
    EXPECT_FALSE(emptyBuf.pick_datagram(temp));
    
    // 测试最大容量
    Buffer smallBuf(8, 16); // 小的初始空间
    smallBuf.set_sep(Buffer::None);

    const std::string largeStr(20, 'A'); // 大于初始空间
    smallBuf.append_with_sep(largeStr);

    EXPECT_EQ(smallBuf.readable(), largeStr.size());
    EXPECT_EQ(smallBuf.writable(), 0);
}


// TAG: 移动语义测试
TEST(BufferTest, MoveSemantics) {
    Buffer buf1;
    buf1.append_with_sep("Data to move");
    
    const char* data = buf1.cbegin() + buf1.prependable();
    
    // 移动构造
    Buffer buf2(std::move(buf1));
    EXPECT_EQ(buf2.retrieve_all_as_string(), "Data to move");
    EXPECT_EQ(buf2.cbegin() + buf2.prependable(), data);
    
    // 原对象状态 - 应为空
    EXPECT_EQ(buf1.readable(), 0);
}

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}