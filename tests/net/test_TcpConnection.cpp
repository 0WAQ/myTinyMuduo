#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/Socket.h"
#include "mymuduo/net/Channel.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/SocketOps.h"
#include "mymuduo/net/callbacks.h"

#include <asm-generic/socket.h>
#include <csignal>
#include <cstddef>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtest/gtest.h>

using namespace mymuduo;
using namespace mymuduo::net;


#define set(conn, name, ...) \
    conn->set_##name##_callback([this](const TcpConnectionPtr& conn, ##__VA_ARGS__) { \
        ++_##name##_callback_count; \
    })

#define set_connection(conn)       set(conn, connection)
#define set_message(conn)          set(conn, message, Buffer* buf, Timestamp t)
#define set_write_complete(conn)   set(conn, write_complete)
#define set_close(conn)            set(conn, close)
#define set_high_water_mark1(conn) set(conn, high_water_mark, size_t len);

#define set_all(conn) \
    do { \
        set_connection(conn); \
        set_message(conn); \
        set_write_complete(conn); \
        set_close(conn); \
        set_high_water_mark1(conn); \
    } while(0)

namespace mymuduo {
namespace net {

// 测试访问接口类
class TcpConnectionAccessor {
public:
    static std::unique_ptr<Socket>& socket(TcpConnectionPtr& conn) {
        return std::ref(conn->_M_sock);
    }
};

}
}

namespace {

// 测试基类
class TcpConnectionTest : public ::testing::Test {
protected:
    TcpConnectionTest() = default;
    ~TcpConnectionTest() override = default;

    void SetUp() override {
        _loop = std::make_shared<EventLoop>();

        // 使用 socketpair 模拟 socket 连接
        ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, _socketfd), 0);
    
        int flags = ::fcntl(_socketfd[0], F_GETFL, 0);
        ::fcntl(_socketfd[0], F_SETFL, flags | O_NONBLOCK);
        flags = fcntl(_socketfd[1], F_GETFL, 0);
        ::fcntl(_socketfd[1], F_SETFL, flags | O_NONBLOCK);
    }

    void TearDown() override {
        sockets::close(_socketfd[1]);
    }

protected:
    std::shared_ptr<TcpConnection> createConn(bool isET) {
        _conn = std::make_shared<TcpConnection>(
            _loop.get(), 1, "TcpConnectionTest", _socketfd[0],
            InetAddress{}, InetAddress{}, isET
        );

        return _conn;
    }

    void writeToServer(const std::string& msg) {
        ssize_t n = ::write(_socketfd[1], msg.data(), msg.size());
        ASSERT_EQ(n, msg.size());
    }

    std::string readFromServer(size_t len) {
        char buf[1024] = {0};
        ssize_t n = ::read(_socketfd[1], buf, len);
        return std::string(buf, n);
    }

protected:
    int _socketfd[2];
    std::shared_ptr<EventLoop> _loop;
    std::shared_ptr<TcpConnection> _conn;

    std::atomic<int> _connection_callback_count { 0 };
    std::atomic<int> _message_callback_count { 0 };
    std::atomic<int> _write_complete_callback_count { 0 };
    std::atomic<int> _close_callback_count { 0 };
    std::atomic<int> _high_water_mark_callback_count { 0 };
};


// TAG: 连接生命周期测试
TEST_F(TcpConnectionTest, ConnectionLifecycle) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->destroyed();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);
    
    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: LT模式下数据接收测试
TEST_F(TcpConnectionTest, LT_DataReceive) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);
    
    std::string msg { "TcpConnectionTest.LT_DataReceive" };
    conn->set_message_callback([&](const TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
        ASSERT_EQ(buf->readable(), msg.size());
        ASSERT_EQ(buf->retrieve_all_as_string(), msg);
        ++_message_callback_count;
    });

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    writeToServer(msg);
    _loop->loop_once();

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(1, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);


    conn->destroyed();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);

    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(1, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: LT模式下数据发送测试
TEST_F(TcpConnectionTest, LT_DataSend) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);
    
    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
    
    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);
    
    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);


    std::string msg { "TcpConnectionTest.LT_DataSend" };    
    conn->send(msg);
    _loop->loop_once();     // 处理 write_complete_callback 任务

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(1, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    std::string res = readFromServer(msg.size());
    ASSERT_EQ(msg, res);

    conn->destroyed();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);

    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(1, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: ET模式下数据接收测试
TEST_F(TcpConnectionTest, ET_DataReceiveFull) {
    auto conn = createConn(true);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);
    
    std::string msg { "TcpConnectionTest.ET_DataReceiveFull" };
    conn->set_message_callback([&](const TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
        ASSERT_EQ(buf->readable(), msg.size());
        ASSERT_EQ(buf->retrieve_all_as_string(), msg);
        ++_message_callback_count;
    });

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
    
    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);
    
    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);


    writeToServer(msg);
    _loop->loop_once();

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(1, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);


    conn->destroyed();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);

    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(1, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: 强制关闭连接测试
TEST_F(TcpConnectionTest, ForceClose) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->force_close();
    _loop->loop_once();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);
    
    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(1, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: 带延时的强制关闭连接测试
TEST_F(TcpConnectionTest, ForceCloseWithDelay) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    bool timeout5ms = false, timeout15ms = false;
    _loop->run_after(5ms, [&timeout5ms] {
        timeout5ms = true;
    });

    _loop->run_after(15ms, [this, &timeout15ms] {
        timeout15ms = true;
        _loop->quit();
    });

    conn->set_close_callback([&](const TcpConnectionPtr& conn) {
        ASSERT_TRUE(timeout5ms);
        ASSERT_FALSE(timeout15ms);
        ++_close_callback_count;
    });

    conn->force_close_with_delay(10ms);
    _loop->loop();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);
    
    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(1, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}


// TAG: 高水位线测试
TEST_F(TcpConnectionTest, HighWaterMark) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);

    const size_t water_mark = 128;
    conn->set_high_water_mark(water_mark);
    conn->set_high_water_mark_callback([this, water_mark](const TcpConnectionPtr& conn, size_t len) {
        ASSERT_GE(len, water_mark);
        ++_high_water_mark_callback_count;
    });

    // 设置内核缓冲区大小, 测试结果可能会随不同内核版本而变化
    int send_buf = 4096;
    ASSERT_EQ(::setsockopt(_socketfd[0], SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf)), 0);

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    std::string msg(send_buf * 2, 'x');
    conn->send(msg);
    _loop->loop_once(10ms);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(1, _high_water_mark_callback_count);

    conn->destroyed();
    ASSERT_EQ(conn->state(), TcpConnection::kDisConnected);
    
    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(1, _high_water_mark_callback_count);
}


// TAG: 错误处理测试
TEST_F(TcpConnectionTest, HandlerWriteError) {
    auto conn = createConn(false);
    ASSERT_EQ(conn->state(), TcpConnection::kConnecting);
    set_all(conn);

    ASSERT_EQ(0, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);

    conn->established();
    ASSERT_EQ(conn->state(), TcpConnection::kConnected);

    ASSERT_EQ(1, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(0, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
    
    // 提前关闭 serv_sock
    sockets::close(_socketfd[1]);
    
    std::string msg { "TcpConnectionTest.HandlerWriteError" };
    conn->send(msg);
    _loop->loop_once();

    ASSERT_EQ(2, _connection_callback_count);
    ASSERT_EQ(0, _message_callback_count);
    ASSERT_EQ(0, _write_complete_callback_count);
    ASSERT_EQ(1, _close_callback_count);
    ASSERT_EQ(0, _high_water_mark_callback_count);
}

} // namespace

int main(int argc, char** argv) {
    ::signal(SIGPIPE, SIG_IGN);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}