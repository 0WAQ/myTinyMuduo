#include "mymuduo/base/Logger.h"
#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/Connector.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/SocketOps.h"
#include "mymuduo/net/Channel.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <future>
#include <memory>
#include <thread>
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <gtest/gtest.h>

using namespace mymuduo;
using namespace mymuduo::net;

namespace {

class ConnectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        _clnt_loop = std::make_shared<EventLoop>();
        _server_addr = InetAddress("127.0.0.1");
    }

    void TearDown() override {
        if (_serv_loop) {
            _serv_loop->run_in_loop([this] {
                _serv_loop->quit();
            });
        }
        if (_serv_thread.joinable()) {
            _serv_thread.join();
        }
        if (_connector) {
            _connector->stop();
        }
    }

protected:
    uint16_t run_server(std::function<void(Timestamp t)> func) {
        std::promise<uint16_t> port_promise;
        auto port_future = port_promise.get_future();

        _serv_thread = std::thread([&port_promise, func, this] {
            int listenfd = sockets::create_non_blocking_fd();
            ASSERT_GT(listenfd, 0);
            sockets::setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, true);
            sockets::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, true);
            sockets::bind(listenfd, _server_addr.addr());
            sockets::listen(listenfd);

            // 获取绑定的端口
            uint16_t port = ntohs(sockets::get_local_addr(listenfd).sin_port);
            port_promise.set_value(port);

            _serv_loop = std::make_shared<EventLoop>();

            Channel channel(_serv_loop.get(), listenfd);
            channel.set_read_callback([&func, listenfd, this](Timestamp t) {
                InetAddress clnt_addr;
                int sockfd = sockets::accept(listenfd, clnt_addr.addr());
                ASSERT_GT(sockfd, 0);
                if (func) {
                    func(t);
                }
            });

            channel.set_read_events();

            _serv_loop->loop(10ms);
        });

        return port_future.get();
    }

protected:
    std::shared_ptr<EventLoop> _clnt_loop;
    std::shared_ptr<EventLoop> _serv_loop;
    
    InetAddress _server_addr;
    std::shared_ptr<Connector> _connector;

    std::thread _serv_thread;
};

template <typename Pred>
bool wait_for(Pred pred, std::chrono::steady_clock::duration timeout = 2000ms) {
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

// TAG
TEST_F(ConnectorTest, ConnectsToServer) {
    std::atomic<bool> serverConnected { false };
    std::atomic<bool> clientConnected { false };

    uint16_t port = run_server([&](Timestamp t) {
        serverConnected.store(true);
    });

    _connector = std::make_shared<Connector>(_clnt_loop.get(), InetAddress("127.0.0.1", port));
    _connector->set_new_connection_callback([&](int sockfd) {
        clientConnected.store(true);
    });
    _connector->start();
    _clnt_loop->loop_once(10ms);

    ASSERT_TRUE(wait_for([&] {
        return serverConnected.load() && clientConnected.load();
    }));
}


// TAG
TEST_F(ConnectorTest, RetryAfterConnectionFailure) {
    std::atomic<bool> connected { false };
    std::atomic<int> retryCount{0};
    int maxAttempts = 3;

    _server_addr = InetAddress("127.0.0.1", 9999);

    _connector = std::make_shared<Connector>(_clnt_loop.get(), _server_addr);
    _connector->set_new_connection_callback([&](int sockfd) {
        connected.store(true);
    });
    _connector->set_retry_callback([&](int sockfd) {
        retryCount++;
        
        // 在最后一次重试前启动服务器
        if (retryCount == maxAttempts - 1) {
            run_server([](Timestamp t) {

            });
        }
    });

    std::thread t([&] {
        ASSERT_TRUE(wait_for([&] {
            return connected.load();
        }));
    
        ASSERT_TRUE(wait_for([&]{
            return retryCount.load() >= maxAttempts - 1;
        }));

        _clnt_loop->run_in_loop([&]{
            _clnt_loop->quit();
        });
    });

    _connector->start();
    _clnt_loop->loop();

    if (t.joinable()) {
        t.join();
    }
}


// TAG
TEST_F(ConnectorTest, StopPreventsConnection) {
    std::atomic<bool> serverConnected { false };
    std::atomic<bool> clientConnected { false };

    auto port = run_server([&](Timestamp t) {
        serverConnected.store(true);
    });

    _connector = std::make_shared<Connector>(_clnt_loop.get(), InetAddress("127.0.0.1", port));
    _connector->set_new_connection_callback([&](int sockfd) {
        clientConnected.store(true);
    });  

    _connector->start();
    _connector->stop();
    
    std::thread t([&] {
        std::this_thread::sleep_for(100ms);
        _clnt_loop->run_in_loop([&] {
            _clnt_loop->quit();
        });
    });

    _clnt_loop->loop();

    if (t.joinable()) {
        t.join();
    }

    EXPECT_TRUE(serverConnected.load());
    EXPECT_FALSE(clientConnected.load());
}


} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    Logger::set_log_level(Logger::ERROR);
    return RUN_ALL_TESTS();
}