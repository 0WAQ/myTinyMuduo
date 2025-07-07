#include "mymuduo/base/Logger.h"
#include "mymuduo/net/TcpClient.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/TcpConnection.h"
#include "mymuduo/net/TcpServer.h"
#include "mymuduo/net/SocketOps.h"
#include "mymuduo/net/callbacks.h"

#include <cassert>
#include <functional>

using namespace mymuduo;
using namespace mymuduo::net;

namespace mymuduo::net {
namespace __detail {

void remove_connection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queue_in_loop(std::bind(&TcpConnection::destroyed, conn));
}

void remove_connector(const TcpClient::ConnectorPtr& connector) {

}

} // namespace __detail
} // namespace mymuduo::net

TcpClient::TcpClient(EventLoop* loop, const InetAddress& server_addr, const std::string& name)
    : _loop(__detail::check_loop_not_null(loop))
    , _connector(new Connector(loop, server_addr))
    , _name(name)
    , _next_id(1)
    , _retry(false)
    , _connect(true)
    , _connection_callback(default_connection_callback)
    , _message_callback(default_message_callback)
{
    LOG_INFO("TcpClient::TcpClient[%s] - connector [%x]", _name.c_str(), _connector.get());
    _connector->set_new_connection_callback(std::bind(&TcpClient::new_connection, this, std::placeholders::_1));
}

TcpClient::~TcpClient() {
    LOG_INFO("TcpClient::~TcpClient[%s] - connector [%x]", _name.c_str(), _connector.get());

    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> guard(_mutex);
        unique = _connection.unique();
        conn = _connection;
    }

    if (conn) {
        assert(_loop == conn->loop());

        // 不要直接调用 destroyed 销毁连接, 应等待服务器将数据发送完毕
        CloseCallback cb = std::bind(&__detail::remove_connection, _loop, std::placeholders::_1);
        _loop->run_in_loop(std::bind(&TcpConnection::set_close_callback, conn, cb));

        // 其它代码不持有 conn 对象的话直接强制关闭
        if (unique) {
            conn->force_close();
        }
    }
    else {
        _connector->stop();
        _loop->run_after(1ms, std::bind(&__detail::remove_connector, _connector));
    }
}

void TcpClient::connect() {
    _connect.store(true);
    _connector->start();
}

void TcpClient::disconnect() {
    _connect.store(false);
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_connection) {
            _connection->shutdown();
        }
    }
}

void TcpClient::stop() {
    _connect.store(false);
    _connector->stop();
}

void TcpClient::new_connection(int sockfd) {
    _loop->is_loop_thread();
    InetAddress peerAddr(sockets::get_peer_addr(sockfd));
    InetAddress localAddr(sockets::get_local_addr(sockfd));

    char buf[32];
    snprintf(buf, sizeof(buf), ";%s#%d", peerAddr.ip_port().c_str(), _next_id);
    std::string name = _name + buf;

    TcpConnectionPtr conn(new TcpConnection(_loop, _next_id ,name, sockfd, localAddr, peerAddr));
    ++_next_id;

    conn->set_connection_callback(_connection_callback);
    conn->set_message_callback(_message_callback);
    conn->set_write_complete_callback(_write_complete_callback);
    conn->set_close_callback(std::bind(&TcpClient::remove_connection, this, std::placeholders::_1));

    {
        std::lock_guard<std::mutex> guard(_mutex);
        _connection = conn;
    }
    conn->established();
}

void TcpClient::remove_connection(const TcpConnectionPtr& conn) {
    _loop->is_loop_thread();
    assert(_loop == conn->loop());

    {
        std::lock_guard<std::mutex> guard(_mutex);
        assert(_connection == conn);
        _connection.reset();
    }

    _loop->queue_in_loop(std::bind(&TcpConnection::destroyed, conn));
    if (_retry && _connect) {
        LOG_INFO("TcpClient::connect[%s] - Reconnection to %s\n", _name.c_str(), _connector->server_addr().ip_port().c_str());
        _connector->restart();
    }
}
