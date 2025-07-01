#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Acceptor.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/SocketOps.h"

using namespace mymuduo;
using namespace mymuduo::net;

Acceptor::Acceptor(EventLoop* main_loop, const InetAddress &serv_addr, bool reuseport)
    : _M_loop(main_loop), _M_serv_addr(serv_addr)
    , _M_serv_sock(sockets::create_non_blocking_fd()), _M_listening(false)
    , _M_acceptor_channel(_M_loop, _M_serv_sock.fd())
{
    LOG_DEBUG("Acceptor create nonblocking socket, [fd = %d].\n", _M_serv_sock.fd());

    // 设置serv_sock的属性
    _M_serv_sock.set_keep_alive(true);
    _M_serv_sock.set_reuse_addr(true);
    _M_serv_sock.set_reuse_port(reuseport);
    _M_serv_sock.set_tcp_nodelay(true);

    // 绑定且监听
    _M_serv_sock.bind(_M_serv_addr);

    // 设置acceptor_channel_ptr的执行函数为new_connection
    _M_acceptor_channel.set_read_callback(std::bind(&Acceptor::new_connection, this));
}

Acceptor::~Acceptor() {
    _M_acceptor_channel.unset_all_events();
    _M_acceptor_channel.remove();
    _M_serv_sock.close();
}

void Acceptor::listen() {
    _M_listening = true;
    _M_serv_sock.listen();
    _M_acceptor_channel.set_read_events();
}

// 读事件的被调函数, 代表有新连接
void Acceptor::new_connection()
{
    InetAddress clnt_addr;

    int clntfd = _M_serv_sock.accept(clnt_addr);
    if(clntfd < 0) {
        LOG_ERROR("%s:%s:%d accprt errot - errno = %d %s.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        if(errno == EMFILE) {
            LOG_ERROR("%s:%s:%d clntfd reached limit! - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        }
    }

    // 通过回调函数将创建好的clnt_sock传递给TcpServer, 让TcpServer创建Connection对象
    if(_M_new_connection_callback) {
        _M_new_connection_callback(clntfd, clnt_addr);
    }
    else {
        sockets::close(clntfd);
    }
}

