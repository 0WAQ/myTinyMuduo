#include "mymuduo/net/TcpClient.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/Acceptor.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/SocketOps.h"
#include "mymuduo/base/Logger.h"

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <functional>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <semaphore>

using namespace mymuduo;
using namespace mymuduo::net;
using namespace std::chrono_literals;

namespace {

template <typename Pred>
bool wait_for(Pred pred, std::chrono::steady_clock::duration timeout
                            = std::chrono::steady_clock::duration::max())
{
    using namespace std::chrono;
    const auto start = steady_clock::now();
    while (!pred()) {
        if (duration_cast<milliseconds>(steady_clock::now() - start) > timeout) {
            return false;
        }
        std::this_thread::sleep_for(10ms);
    }
    return true;
}

class TcpClientTest : public testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
        if (_client) {
            _client->stop();
        }
        
        if (_clnt_loop) {
            _clnt_loop->run_in_loop([&] {
                _clnt_loop->quit();
            });
        }

        if (_serv_loop) {
            _serv_loop->run_in_loop([&] {
                _serv_loop->quit();
            });
        }
        
        if (_clnt_thread.joinable()) {
            _clnt_thread.join();
        }
        
        if (_serv_thread.joinable()) {
            _serv_thread.join();
        }
    }

    uint16_t start_server(uint16_t port = 0) {
        if (_serv_thread.joinable()) {
            if (_serv_loop) {
                _serv_loop->run_in_loop([&] {
                    _serv_loop->quit();
                });
            }
            _serv_thread.join();
        }

        std::counting_semaphore<1> sem(0);
        std::promise<uint16_t> promise;
        auto future = promise.get_future();

        using namespace std::placeholders;
        _serv_thread = std::thread([&] {
            _serv_loop = std::make_unique<EventLoop>();
            _acceptor = std::make_unique<Acceptor>(_serv_loop.get(),
                                                   InetAddress("127.0.0.1", port),
                                                   true);
            _acceptor->set_new_connection_callback(
                    std::bind(&TcpClientTest::handle_new_connection, this, _1, _2));

            auto port = ntohs(sockets::get_local_addr(_acceptor->socket().fd()).sin_port);
            promise.set_value(port);

            sem.release();
            _acceptor->listen();
            _serv_loop->loop();
        });
        sem.acquire();
        return future.get();
    }

    void stop_server() {
        _serv_loop->run_in_loop([this] {
            _acceptor.reset();
            for (auto& conn : _serv_connections) {
                conn->shutdown();
            }
            _serv_connections.clear();
        });
    }
    
    void handle_new_connection(int sockfd, InetAddress& peerAddr) {
        InetAddress localAddr(sockets::get_local_addr(sockfd));
        char buf[64];
        snprintf(buf, sizeof(buf), "SERV-%s", peerAddr.ip_port().c_str());
        TcpConnectionPtr conn(new TcpConnection(_serv_loop.get(), 0,
                                                   buf, sockfd,
                                                   localAddr, peerAddr));
        
        conn->set_connection_callback([this](const TcpConnectionPtr& conn) {
            if (!conn->connected()) {
                _serv_connections.erase(conn);
            }
        });
        
        conn->set_message_callback([this](const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time) {
            _serv_received_data += buf->retrieve_all_as_string();
            conn->send(_serv_received_data); // 回显数据
        });
        
        conn->set_close_callback([this](const TcpConnectionPtr& conn) {
            _serv_connections.erase(conn);
        });
        
        _serv_connections.insert(conn);
        conn->established();
    }

    void start_client(uint16_t port) {
        std::counting_semaphore<1> sem(0);

        _clnt_thread = std::thread([&] {
            _clnt_loop = std::make_unique<EventLoop>();
            _client = std::make_unique<TcpClient>(_clnt_loop.get(), 
                                                  InetAddress("127.0.0.1", port),
                                                  "TcpClientTest");

            sem.release();
            _clnt_loop->loop();
        });
        sem.acquire();
    }

protected:
    // 客户端
    std::unique_ptr<EventLoop> _clnt_loop;
    std::unique_ptr<TcpClient> _client;
    std::thread _clnt_thread;
    
    // 服务器
    std::unique_ptr<EventLoop> _serv_loop;
    std::unique_ptr<Acceptor> _acceptor;
    std::thread _serv_thread;
    std::set<TcpConnectionPtr> _serv_connections;
    std::string _serv_received_data;
};


// TAG: 测试基本连接
TEST_F(TcpClientTest, ConnectsToServer) {
    auto port = start_server();
    start_client(port);
    
    // 设置连接回调
    bool connected = false;
    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        connected = conn->connected();
    });
    _client->connect();
    
    // 等待连接建立
    ASSERT_TRUE(wait_for([&] {
        return connected;
    }));
    
    // 验证连接存在
    ASSERT_NE(_client->connection(), nullptr);
    ASSERT_TRUE(_client->connection()->connected());
    
    stop_server();
}


// TAG: 测试消息发送和接收
TEST_F(TcpClientTest, SendsAndReceivesMessages) {
    auto port = start_server();
    start_client(port);
    
    std::string client_received;
    _client->set_message_callback([&](const TcpConnectionPtr&, Buffer* buf, TimeStamp) {
        client_received = buf->retrieve_all_as_string();
    });
    _client->connect();
    
    ASSERT_TRUE(wait_for([&] {
        return _client->connection() != nullptr;
    }));
    
    const std::string test_message = "Hello, Server!";
    _client->connection()->send(test_message);
    
    ASSERT_TRUE(wait_for([&] {
        return client_received == test_message;
    }));
    
    stop_server();
}


// TAG: 测试断开连接
TEST_F(TcpClientTest, DisconnectsProperly) {
    auto port = start_server();
    start_client(port);

    enum ConnState {
        Disconnected,
        Connected
    } state = ConnState::Disconnected;

    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            state = ConnState::Connected;
        } else {
            state = ConnState::Disconnected;
        }
    });
    
    _client->connect();
    ASSERT_TRUE(wait_for([&] {
        return state == ConnState::Connected;
    }));
    
    _client->disconnect();
    ASSERT_TRUE(wait_for([&] {
        return state == ConnState::Disconnected;
    }));
    
    stop_server();
}


// TAG: 测试自动重连
TEST_F(TcpClientTest, ReconnectsAutomatically) {
    auto port = start_server();
    start_client(port);
    _client->enable_retry();
    
    // 设置连接状态跟踪
    int connectionCount = 0;
    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            connectionCount++;
        }
    });
    
    _client->connect();
    ASSERT_TRUE(wait_for([&] {
        return connectionCount == 1;
    }));
    
    stop_server();
    ASSERT_TRUE(wait_for([&] {
        return _client->connection() == nullptr;
    }));
    
    // 重启服务器
    start_server(port);
    ASSERT_TRUE(wait_for([&] {
        return connectionCount == 2;
    }));
    
    stop_server();
}


// TAG: 测试有连接时的析构
TEST_F(TcpClientTest, DestructsWithConnection) {
    auto port = start_server();
    start_client(port);
    
    _client->connect();
    ASSERT_TRUE(wait_for([&] {
        return _client->connection() != nullptr;
    }));

    _client.reset();

    TcpConnectionPtr conn = _client->connection();
    ASSERT_TRUE(wait_for([&] {
        return !conn->connected();
    }));
    
    stop_server();
}


// TAG: 测试无连接时的析构
TEST_F(TcpClientTest, DestructsWithoutConnection) {
    start_client(9999); // 无效地址
    ASSERT_TRUE(true);
}


// TAG: 测试服务器关闭连接
TEST_F(TcpClientTest, HandlesServerClose) {
    auto port = start_server();
    start_client(port);
    
    // 设置连接状态跟踪
    bool disconnected = false;
    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        if (!conn->connected()) {
            disconnected = true;
        }
    });
    
    _client->connect();
    ASSERT_TRUE(wait_for([&] {
        return _client->connection() != nullptr;
    }));
    
    stop_server();
    ASSERT_TRUE(wait_for([&] {
        return disconnected;
    }));
    
    // 验证连接已关闭
    ASSERT_EQ(_client->connection(), nullptr);
}


// TAG: 测试网络错误
TEST_F(TcpClientTest, HandlesNetworkError) {
    auto port = start_server();
    start_client(port);
    
    // 设置连接状态跟踪
    bool disconnected = false;
    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        if (!conn->connected()) {
            disconnected = true;
        }
    });
    
    _client->connect();
    ASSERT_TRUE(wait_for([&] {
        return _client->connection() != nullptr;
    }));
    
    // 关闭服务器端连接
    _serv_loop->run_in_loop([this] {
        if (!_serv_connections.empty()) {
            (*_serv_connections.begin())->shutdown();
        }
    });
    
    // 等待连接断开
    ASSERT_TRUE(wait_for([&] {
        return disconnected;
    }));
    
    stop_server();
}


// TAG: 测试连接失败
TEST_F(TcpClientTest, HandlesConnectionFailure) {
    start_client(9999);
    
    // 设置连接状态跟踪
    bool connected = false;
    bool disconnected = false;
    _client->set_connection_callback([&](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            connected = true;
        } else {
            disconnected = true;
        }
    });
    
    _client->connect();
    std::this_thread::sleep_for(500ms);
    
    // 验证连接失败
    ASSERT_FALSE(connected);
    ASSERT_FALSE(disconnected);
    ASSERT_EQ(_client->connection(), nullptr);
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    Logger::instance().set_log_level(Logger::ERROR);
    Logger::instance().set_output([](const char* data, size_t len) {
        std::fprintf(stdout, data, len);
    });
    return RUN_ALL_TESTS();
}