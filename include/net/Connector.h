#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "base/TimeStamp.h"
#include "base/noncopyable.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/TcpConnection.h"

#include <atomic>
#include <gtest/gtest.h>
#include <memory>

using namespace std::chrono_literals;

namespace mymuduo {
namespace net {

class Channel;
class EventLoop;

class Connector : noncopyable
                , public std::enable_shared_from_this<Connector> 
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    enum State {
        kDisConnected,
        kConnecting,
        kConnected
    };

public:
    Connector(EventLoop* loop, const InetAddress& server_addr);
    ~Connector();

    void start();
    void restart();
    void stop();

    const InetAddress& server_addr() const { return _M_server_addr; }

    void set_new_connection_callback(NewConnectionCallback cb) { _M_new_connection_callback = std::move(cb); }

private:

    static constexpr TimeDuration kMaxRetryDelay = 30s;
    static constexpr TimeDuration kInitRetryDelay = 500ms;

    void start_in_loop();
    void stop_in_loop();
    void connect();
    void connecting(int sockfd);
    void handle_write();
    void handle_error();
    void retry(int sockfd);
    int remove_and_reset_channel();
    void reset_channel();

private:
    EventLoop* _M_loop;
    InetAddress _M_server_addr;
    std::unique_ptr<Channel> _M_channel;
    State _M_state;
    TimeDuration _M_retry_delay;
    std::atomic<bool> _M_connect;
    NewConnectionCallback _M_new_connection_callback;
};

} // namespace net
} // namespace mymuduo

#endif // CONNECTOR_H
