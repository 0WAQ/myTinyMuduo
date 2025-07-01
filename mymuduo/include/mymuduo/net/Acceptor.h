/**
 * 
 * Acceptor头文件
 * 
 */
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "mymuduo/base/noncopyable.h"
#include "mymuduo/net/InetAddress.h"
#include "mymuduo/net/Socket.h"
#include "mymuduo/net/Channel.h"

namespace mymuduo {
namespace net {

class EventLoop;

class Acceptor : noncopyable {
public:
    using CreateConnCallback = std::function<void(int, InetAddress&)>;

public:
    Acceptor(EventLoop* main_loop, const InetAddress &serv_addr, bool reuseport);
    ~Acceptor();

    void listen();

    void new_connection();

    bool listenning() const { return _M_listening; }
    const Socket& socket() const { return _M_serv_sock; }
    const InetAddress& listen_addr() const { return _M_serv_addr; }

    void set_new_connection_callback(CreateConnCallback func) {
        _M_new_connection_callback = std::move(func);
    }

private:
    EventLoop* _M_loop;
    InetAddress _M_serv_addr;
    Socket _M_serv_sock;
    Channel _M_acceptor_channel;

    bool _M_listening;
    bool _M_stopping;

    // 创建Connection对象的回调函数, 将回调TcpServer::create_connection
    CreateConnCallback _M_new_connection_callback;
};

} // namespace net
} // namespace mymuduo

#endif // ACCEPTOR_H