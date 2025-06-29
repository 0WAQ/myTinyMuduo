#ifndef TCPCLIENT_H
#define TCPCLIENT_H

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

    EventLoop* loop() const { return _M_loop; }
    const std::string& name() const { return _M_name; }
    bool retry() const { return _M_retry; }
    void enable_retry() { _M_retry = true; }
    TcpConnectionPtr connection() {
        std::lock_guard<std::mutex> guard(_M_mutex);
        return _M_connection;
    }

    void set_connection_callback(ConnectionCallback cb) { _M_connection_callback = std::move(cb); }
    void set_message_callback(MessageCallback cb) { _M_message_callback = std::move(cb); }
    void set_write_complete_callback(WriteCompleteCallback cb) { _M_write_complete_callback = std::move(cb); }

private:
    void new_connection(int sockfd);
    void remove_connection(const TcpConnectionPtr& conn);

private:
    EventLoop* _M_loop;
    ConnectorPtr _M_connector;
    TcpConnectionPtr _M_connection;
    const std::string _M_name;

    std::atomic<bool> _M_retry;
    std::atomic<bool> _M_connect;

    int _M_next_id;
    std::mutex _M_mutex;

    ConnectionCallback _M_connection_callback;
    MessageCallback _M_message_callback;
    WriteCompleteCallback _M_write_complete_callback;
};

} // namespace net
} // namespace mymuduo

#endif // TCPCLIENT_H
