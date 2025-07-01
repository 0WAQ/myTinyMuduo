#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <atomic>
#include <memory>
#include <functional>

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/base/Timestamp.h"
#include "mymuduo/net/InetAddress.h"

using namespace std::chrono_literals;

namespace mymuduo {
namespace net {

class Channel;
class EventLoop;

/**
 * @brief 用于客户端主动发起连接, 负责连接建立过程的临时管理
 */
class Connector : noncopyable
                , public std::enable_shared_from_this<Connector> 
{
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;
    using RetryCallback = std::function<void(int sockfd)>;

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
    const bool connecting() const { return _M_connect.load(); }

    void set_new_connection_callback(NewConnectionCallback cb) { _M_new_connection_callback = std::move(cb); }
    void set_retry_callback(RetryCallback cb) { _M_retry_callback = std::move(cb); }

private:

    static constexpr TimeDuration kMaxRetryDelay = 30s;
    static constexpr TimeDuration kInitRetryDelay = 500ms;

    void start_in_loop();
    void stop_in_loop();
    void connect();
    void do_connect(int sockfd);
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
    RetryCallback _M_retry_callback;
};

} // namespace net
} // namespace mymuduo

#endif // CONNECTOR_H
