#ifndef MYMUDUO_NET_ACCEPTOR_H
#define MYMUDUO_NET_ACCEPTOR_H

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

    bool listenning() const { return _listening; }
    const Socket& socket() const { return _serv_sock; }
    const InetAddress& listen_addr() const { return _serv_addr; }

    void set_new_connection_callback(CreateConnCallback func) {
        _new_connection_callback = std::move(func);
    }

private:
    EventLoop* _loop;
    InetAddress _serv_addr;
    Socket _serv_sock;
    Channel _acceptor_channel;

    bool _listening;
    bool _stopping;

    // 创建Connection对象的回调函数, 将回调TcpServer::create_connection
    CreateConnCallback _new_connection_callback;
};

} // namespace net
} // namespace mymuduo

#endif // MYMUDUO_NET_ACCEPTOR_H