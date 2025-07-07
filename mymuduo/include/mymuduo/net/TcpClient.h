#ifndef MYMUDUO_NET_TCPCLIENT_H
#define MYMUDUO_NET_TCPCLIENT_H

#include <memory>
#include <mutex>

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/callbacks.h"
#include "mymuduo/net/Connector.h"

namespace mymuduo {
namespace net {

class EventLoopl;

class TcpClient : noncopyable {
public:
    using ConnectorPtr = std::shared_ptr<Connector>;

public:
    TcpClient(EventLoop* loop, const InetAddress& server_addr, const std::string& name);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    EventLoop* loop() const { return _loop; }
    const std::string& name() const { return _name; }
    const bool retry() const { return _retry; }
    void enable_retry() { _retry = true; }
    TcpConnectionPtr connection() {
        std::lock_guard<std::mutex> guard(_mutex);
        return _connection;
    }

    void set_connection_callback(ConnectionCallback cb) { _connection_callback = std::move(cb); }
    void set_message_callback(MessageCallback cb) { _message_callback = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { _write_complete_callback = std::move(cb); }

private:
    void new_connection(int sockfd);
    void remove_connection(const TcpConnectionPtr& conn);

private:
    EventLoop* _loop;
    ConnectorPtr _connector;
    TcpConnectionPtr _connection;
    const std::string _name;

    std::atomic<bool> _retry;
    std::atomic<bool> _connect;

    int _next_id;
    std::mutex _mutex;

    ConnectionCallback _connection_callback;
    MessageCallback _message_callback;
    WriteCompleteCallback _write_complete_callback;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_TCPCLIENT_H
