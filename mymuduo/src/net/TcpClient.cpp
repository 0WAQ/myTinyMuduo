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
    : _M_loop(__detail::check_loop_not_null(loop))
    , _M_connector(new Connector(loop, server_addr))
    , _M_name(name)
    , _M_next_id(1)
    , _M_retry(false)
    , _M_connect(true)
    , _M_connection_callback(default_connection_callback)
    , _M_message_callback(default_message_callback)
{
    LOG_INFO("TcpClient::TcpClient[%s] - connector [%x]", _M_name.c_str(), _M_connector.get());
    _M_connector->set_new_connection_callback(std::bind(&TcpClient::new_connection, this, std::placeholders::_1));
}

TcpClient::~TcpClient() {
    LOG_INFO("TcpClient::~TcpClient[%s] - connector [%x]", _M_name.c_str(), _M_connector.get());

    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> guard(_M_mutex);
        unique = _M_connection.unique();
        conn = _M_connection;
    }

    if (conn) {
        assert(_M_loop == conn->loop());

        // 不要直接调用 destroyed 销毁连接, 应等待服务器将数据发送完毕
        CloseCallback cb = std::bind(&__detail::remove_connection, _M_loop, std::placeholders::_1);
        _M_loop->run_in_loop(std::bind(&TcpConnection::set_close_callback, conn, cb));

        // 其它代码不持有 conn 对象的话直接强制关闭
        if (unique) {
            conn->force_close();
        }
    }
    else {
        _M_connector->stop();
        _M_loop->run_after(1ms, std::bind(&__detail::remove_connector, _M_connector));
    }
}

void TcpClient::connect() {
    _M_connect.store(true);
    _M_connector->start();
}

void TcpClient::disconnect() {
    _M_connect.store(false);
    {
        std::lock_guard<std::mutex> guard(_M_mutex);
        if (_M_connection) {
            _M_connection->shutdown();
        }
    }
}

void TcpClient::stop() {
    _M_connect.store(false);
    _M_connector->stop();
}

void TcpClient::new_connection(int sockfd) {
    _M_loop->is_loop_thread();
    InetAddress peerAddr(sockets::get_peer_addr(sockfd));
    InetAddress localAddr(sockets::get_local_addr(sockfd));

    char buf[32];
    snprintf(buf, sizeof(buf), ";%s#%d", peerAddr.ip_port().c_str(), _M_next_id);
    std::string name = _M_name + buf;

    TcpConnectionPtr conn(new TcpConnection(_M_loop, _M_next_id ,name, sockfd, localAddr, peerAddr));
    ++_M_next_id;

    conn->set_connection_callback(_M_connection_callback);
    conn->set_message_callback(_M_message_callback);
    conn->set_write_complete_callback(_M_write_complete_callback);
    conn->set_close_callback(std::bind(&TcpClient::remove_connection, this, std::placeholders::_1));

    {
        std::lock_guard<std::mutex> guard(_M_mutex);
        _M_connection = conn;
    }
    conn->established();
}

void TcpClient::remove_connection(const TcpConnectionPtr& conn) {
    _M_loop->is_loop_thread();
    assert(_M_loop == conn->loop());

    {
        std::lock_guard<std::mutex> guard(_M_mutex);
        assert(_M_connection == conn);
        _M_connection.reset();
    }

    _M_loop->queue_in_loop(std::bind(&TcpConnection::destroyed, conn));
    if (_M_retry && _M_connect) {
        LOG_INFO("TcpClient::connect[%s] - Reconnection to %s\n", _M_name.c_str(), _M_connector->server_addr().ip_port().c_str());
        _M_connector->restart();
    }
}
