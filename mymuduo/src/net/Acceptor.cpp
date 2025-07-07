#include "mymuduo/base/Logger.h"
#include "mymuduo/net/Acceptor.h"
#include "mymuduo/net/EventLoop.h"
#include "mymuduo/net/SocketOps.h"

using namespace mymuduo;
using namespace mymuduo::net;

Acceptor::Acceptor(EventLoop* main_loop, const InetAddress &serv_addr, bool reuseport)
    : _loop(main_loop), _serv_addr(serv_addr)
    , _serv_sock(sockets::create_non_blocking_fd()), _listening(false)
    , _acceptor_channel(_loop, _serv_sock.fd())
{
    LOG_DEBUG("Acceptor create nonblocking socket, [fd = %d].\n", _serv_sock.fd());

    // 设置serv_sock的属性
    _serv_sock.set_keep_alive(true);
    _serv_sock.set_reuse_addr(true);
    _serv_sock.set_reuse_port(reuseport);
    _serv_sock.set_tcp_nodelay(true);

    // 绑定且监听
    _serv_sock.bind(_serv_addr);

    // 设置acceptor_channel_ptr的执行函数为new_connection
    _acceptor_channel.set_read_callback(std::bind(&Acceptor::new_connection, this));
}

Acceptor::~Acceptor() {
    _acceptor_channel.unset_all_events();
    _acceptor_channel.remove();
    _serv_sock.close();
}

void Acceptor::listen() {
    _listening = true;
    _serv_sock.listen();
    _acceptor_channel.set_read_events();
}

// 读事件的被调函数, 代表有新连接
void Acceptor::new_connection()
{
    InetAddress clnt_addr;

    int clntfd = _serv_sock.accept(clnt_addr);
    if(clntfd < 0) {
        LOG_ERROR("%s:%s:%d accprt errot - errno = %d %s.\n", 
            __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        if(errno == EMFILE) {
            LOG_ERROR("%s:%s:%d clntfd reached limit! - errno = %d %s.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        }
    }

    // 通过回调函数将创建好的clnt_sock传递给TcpServer, 让TcpServer创建Connection对象
    if(_new_connection_callback) {
        _new_connection_callback(clntfd, clnt_addr);
    }
    else {
        sockets::close(clntfd);
    }
}

