#include "base/TimeStamp.h"
#include "net/EventLoopThread.h"
#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/TcpConnection.h"
#include "net/SocketOps.h"
#include "net/callbacks.h"

#include <cerrno>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

#include <gtest/gtest.h>

using namespace mymuduo;
using namespace mymuduo::net;

namespace {

class TcpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        _thread = std::thread([this] {
            std::shared_ptr<EventLoop> loop(new EventLoop);
            std::shared_ptr<TcpServer> server(new TcpServer(
                    loop.get(), InetAddress{ 5678 }, "TcpServerTest"
                ));

            server->set_connection_callback([this](const TcpConnectionPtr& conn) {
                // 连接建立
                if (!_connection_callback_called) {
                    _connection_callback_called = true;
                    _conn = conn;
                    _cv.notify_one();
                }

                // 连接关闭
                if (!conn->connected()) {
                    _disconnection_callback_called = true;
                    _cv.notify_one();
                }
            });

            server->set_message_callback([this](const TcpConnectionPtr& conn, Buffer* buf, TimeStamp t) {
                _message_callback_called = true;
                _message_received = buf->retrieve_all_as_string();
                _cv.notify_one();
            });

            _main_loop = loop;
            _server = server;

            server->start();
            
            _cv.notify_one();
            loop->loop();
        });

        {
            std::unique_lock<std::mutex> lock { _mtx };
            _cv.wait(lock);
        }
    }

    void TearDown() override {
        if (_conn && _conn->connected()) {
            _conn->force_close();
        }

        if (auto loop = _main_loop.lock()) {
            loop->run_in_loop([loop] {
                loop->quit();
            });
        }

        if (_thread.joinable()) {
            _thread.join();
        }
    }

protected:

    std::weak_ptr<EventLoop> _main_loop;
    std::weak_ptr<TcpServer> _server;
    std::thread _thread;

    std::mutex _mtx;
    std::condition_variable _cv;

    std::shared_ptr<TcpConnection> _conn;

    std::atomic<bool> _connection_callback_called { false };
    std::atomic<bool> _disconnection_callback_called { false };
    std::atomic<bool> _message_callback_called { false };
    std::string _message_received;
};


// TAG: 连接管理测试
TEST_F(TcpServerTest, AcceptConnectionAndMessage) {
    auto server = _server.lock();

    // 创建客户端 socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GT(sockfd, 0);

    InetAddress clnt_addr("127.0.0.1", server->listen_addr().port());
    int ret = ::connect(sockfd, clnt_addr.addr(), sizeof(sockaddr));
    ASSERT_TRUE(ret == 0);

    // 等待连接建立
    {
        std::unique_lock<std::mutex> lock { _mtx };
        _cv.wait(lock);
    }

    ASSERT_TRUE(_connection_callback_called);
    ASSERT_TRUE(_conn->connected());

    // 发送消息
    const std::string msg = "TcpServerTest.AcceptConnectionAndMessage";
    ssize_t sent = write(sockfd, msg.data(), msg.size());
    ASSERT_EQ(sent, msg.size());

    // 等待消息回调
    {
        std::unique_lock<std::mutex> lock { _mtx };
        _cv.wait(lock);
    }
    ASSERT_TRUE(_message_callback_called);
    ASSERT_EQ(_message_received, msg);

    // 关闭客户端
    sockets::close(sockfd);
    
    // 等待关闭连接回调
    {
        std::unique_lock<std::mutex> lock { _mtx };
        _cv.wait(lock);
    }
    ASSERT_TRUE(_disconnection_callback_called);
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
