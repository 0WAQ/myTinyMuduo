#include "../include/Acceptor.hpp"

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, const uint16_t port) 
    : _M_loop_ptr(loop), _M_serv_sock(create_non_blocking_fd())
    , _M_acceptor_channel(_M_loop_ptr, _M_serv_sock.get_fd())
{
    // 初始化serv_sock的地址
    InetAddress serv_addr(ip, port);

    // 设置serv_sock的属性
    _M_serv_sock.set_keep_alive(1);
    _M_serv_sock.set_reuse_addr(1);
    _M_serv_sock.set_reuse_port(1);
    _M_serv_sock.set_tcp_nodelay(1);

    // 绑定且监听
    _M_serv_sock.bind(serv_addr);
    _M_serv_sock.listen();

    // 监听读事件
    _M_acceptor_channel.set_read_events();
    // 设置acceptor_channel_ptr的执行函数为new_connection
    _M_acceptor_channel.set_read_callback(std::bind(&Acceptor::new_connection, this));

}

// 创建连接的回调函数, 调用TcpServer中的create_connection
void Acceptor::set_create_connection_callback(CreateConnCallback func) {
    _M_create_connection_callback = std::move(func);
} 

// 读事件的被调函数, 代表有新连接
void Acceptor::new_connection()
{
    InetAddress clnt_addr;
    std::unique_ptr<Socket> clnt_sock_ptr(new Socket(_M_serv_sock.accept(clnt_addr)));
    clnt_sock_ptr->set_ip_port(clnt_addr.get_ip(), clnt_addr.get_port());

    // 通过回调函数将创建好的clnt_sock传递给TcpServer, 让TcpServer创建Connection对象
    _M_create_connection_callback(std::move(clnt_sock_ptr));
}

Acceptor::~Acceptor() { }