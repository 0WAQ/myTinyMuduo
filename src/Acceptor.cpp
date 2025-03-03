#include "Acceptor.h"
#include "InetAddress.h"
#include "Logger.h"

namespace mymuduo
{

namespace __detail
{
    /**
     * @brief 创建非阻塞的sockfd
     */
    int create_non_blocking_fd()
    {
        int listen_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if(listen_fd < 0) {
            LOG_ERROR("%s:%s:%d listen_fd create error:%d.\n", 
                __FILE__, __FUNCTION__, __LINE__, errno);
        }
        return listen_fd;
    }

} // namespace __detail

Acceptor::Acceptor(EventLoop* loop, const InetAddress &serv_addr, bool reuseport) :
    _M_loop(loop), _M_serv_sock(__detail::create_non_blocking_fd()), _M_listenning(false),
    _M_acceptor_channel(_M_loop, _M_serv_sock.get_fd())
{
    LOG_DEBUG("Acceptor create nonblocking socket, [fd = %d].\n", _M_serv_sock.get_fd());

    // 设置serv_sock的属性
    _M_serv_sock.set_keep_alive(true);
    _M_serv_sock.set_reuse_addr(true);
    _M_serv_sock.set_reuse_port(true);
    _M_serv_sock.set_tcp_nodelay(true);

    // 绑定且监听
    _M_serv_sock.bind(serv_addr);

    // 设置acceptor_channel_ptr的执行函数为new_connection
    _M_acceptor_channel.set_read_callback(std::bind(&Acceptor::new_connection, this));

}

Acceptor::~Acceptor() {
    _M_acceptor_channel.unset_all_events();
    _M_acceptor_channel.remove();
}

void Acceptor::listen() {
    _M_listenning = true;
    _M_serv_sock.listen();
    _M_acceptor_channel.set_read_events();
}

// 读事件的被调函数, 代表有新连接
void Acceptor::new_connection()
{
    InetAddress clnt_addr;

    int clnt_fd = _M_serv_sock.accept(clnt_addr);
    if(clnt_fd < 0) {
        LOG_ERROR("%s:%s:%d accpet error %d.\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE) {
            LOG_ERROR("%s:%s:%d clnt_fd reached limit!\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }

    // 通过回调函数将创建好的clnt_sock传递给TcpServer, 让TcpServer创建Connection对象
    if(_M_new_connection_callback) {
        _M_new_connection_callback(clnt_fd, clnt_addr);
    }
    else {
        ::close(clnt_fd);
    }
}

} // namespace mymuduo