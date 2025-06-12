/**
 * 
 * Acceptor头文件
 * 
 */
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "Socket.h"
#include "Channel.h"
#include "noncopyable.h"

namespace mymuduo
{

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

    /**
     * @brief
     */
    void listen();

    void new_connection();

    bool listenning() const { return _M_listenning; }

    void set_new_connection_callback(CreateConnCallback func) {
        _M_new_connection_callback = std::move(func);
    }

private:

    EventLoop* _M_loop;
    Socket _M_serv_sock;
    Channel _M_acceptor_channel;

    bool _M_listenning;

    // 创建Connection对象的回调函数, 将回调TcpServer::create_connection
    CreateConnCallback _M_new_connection_callback;
};

} // namespace mymuduo

#endif // ACCEPTOR_H