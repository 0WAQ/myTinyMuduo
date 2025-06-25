/**
 * 
 * Acceptor头文件
 * 
 */
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "base/noncopyable.h"
#include "net/InetAddress.h"
#include "net/Socket.h"
#include "net/Channel.h"

namespace mymuduo {
namespace net {

class EventLoop;

class Acceptor : noncopyable
{
public:

    using CreateConnCallback = std::function<void(int, InetAddress&)>;

public:

    /**
     * @param loop 主事件循环
     */
    Acceptor(EventLoop* loop, const InetAddress &serv_addr, bool reuseport);

    ~Acceptor();

    void listen();
    void stop();

    void new_connection();

    bool listenning() const { return _M_listening; }
    const InetAddress& listen_addr() const { return _M_serv_addr; }

    void set_new_connection_callback(CreateConnCallback func) {
        _M_new_connection_callback = std::move(func);
    }

private:
    void stop_in_loop();

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